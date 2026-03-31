#include "nappluginbridge.h"
#include "project.h"

#include "public.sdk/source/main/pluginfactory.h"
#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "public.sdk/source/vst/utility/stringconvert.h"

#include "vstgui/lib/vstguiinit.h"
#include "public.sdk/source/main/moduleinit.h"

#include "pluginterfaces/base/funknownimpl.h"
#include "base/source/fstreamer.h"

#include <rtti/typeinfo.h>
#include <parameternumeric.h>
#include <parameterdropdown.h>
#include <sdlhelpers.h>
#include <utility/fileutils.h>

#include <functional>

#ifdef WIN32
	#include <windows.h>
	#include <atlstr.h>
#else
	#include <dlfcn.h>
#endif

constexpr const char* app_json = "app.json";

using Steinberg::ModuleInitializer;
using Steinberg::ModuleTerminator;
using Steinberg::getPlatformModuleHandle;

namespace Steinberg
{

	namespace Vst
	{


#ifdef WIN32
		std::string WideToUtf8(LPCWSTR wstr) {
			if (!wstr) return {};
			int size_needed = WideCharToMultiByte(
				CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
			if (size_needed == 0) return {};
			std::string result(size_needed - 1, '\0'); // exclude null
			WideCharToMultiByte(CP_UTF8, 0, wstr, -1,
								&result[0], size_needed, nullptr, nullptr);
			return result;
		}
#endif


		NapPluginBridge::NapPluginBridge ()
		{
		}

		NapPluginBridge::~NapPluginBridge()
		{
			terminate();
		}


		tresult PLUGIN_API NapPluginBridge::initialize (FUnknown* context)
		{

			tresult result = SingleComponentEffect::initialize (context);
			if (result != kResultOk)
				return result;


			if (pluginAudioInput != SpeakerArr::kEmpty)
				addAudioInput  (STR16 ("Input"),  pluginAudioInput);

			if (pluginAudioOutput != SpeakerArr::kEmpty)
				addAudioOutput (STR16 ("Output"), pluginAudioOutput);

			// One single channel event bus
			addEventInput (STR16 ("Event In"), 1);

			parameters.addParameter (STR16("Bypass"), nullptr, 1, 0, ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass, kBypassId);

			nap::utility::ErrorState error;

			mControlThread.start();
			bool napResult = false;
			std::atomic<bool> napInitialized(false);
			mControlThread.enqueue([&]()
			{
				napResult = initializeNAP(mMainThreadQueue, error);
				napInitialized = true;
			});
			while (!napInitialized)
				mMainThreadQueue.process();
			mMainThreadQueue.process(); // Make sure the queue is empty

			if (!napResult)
				return kResultFalse;

			mEventConverter = std::make_unique<nap::SDLEventConverter>(*mSDLInputService);
			mTimer = Timer::create(this, 1000.f / 60.f);

			mControlThread.connectPeriodicTask(mControlSlot);

			return result;
		}


		bool NapPluginBridge::initializeNAP(nap::TaskQueue& mainThreadQueue, nap::utility::ErrorState& errorState)
		{
			mCore = std::make_unique<nap::Core>(mainThreadQueue);

#ifdef WIN32
			HMODULE hModule = nullptr;
			GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, LPCWSTR(app_json), &hModule);
			WCHAR loaderPath[MAX_PATH];
			GetModuleFileName(hModule, loaderPath, MAX_PATH);
			std::string loaderDir = nap::utility::getFileDir(WideToUtf8(loaderPath));
			std::string resourceDir = nap::utility::joinPath({ loaderDir, "..", "Resources" });
#else
			Dl_info info;
			dladdr((void*)(app_json), &info);
			std::string loaderPath = info.dli_fname;
			std::string loaderDir = nap::utility::getFileDir(loaderPath);
			std::string resourceDir = nap::utility::joinPath({ loaderDir, "..", "Resources" });
#endif

			if (!mCore->initializeEngineWithoutProjectInfo(errorState))
				return false;
			mCore->setupPlatformSpecificEnvironment();

			mServices = mCore->initializeServices(errorState);
			if (mServices == nullptr || !mServices->initialized())
			{
				nap::Logger::error(errorState.toString().c_str());
				return false;
			}

			mAudioService = mCore->getService<nap::audio::AudioService>();
			mAudioService->getNodeManager().setInputChannelCount(2);
			mAudioService->getNodeManager().setOutputChannelCount(2);

			std::string app_structure_path = nap::utility::joinPath({ resourceDir, stringAppStructurePath });
			std::string data_dir = nap::utility::getFileDir(app_structure_path);

			if (nap::utility::fileExists(app_structure_path))
			{
				nap::utility::changeDir(data_dir);
				app_structure_path = nap::utility::getFileName(app_structure_path);
				if (!mCore->getResourceManager()->loadFile(app_structure_path, errorState))
				{
					nap::Logger::error("Failed to load app structure: %s", errorState.toString().c_str());
					return false;
				}
				// mCore->getResourceManager()->watchDirectory(data_dir);
			}
			else {
				nap::Logger::error("Failed to load app structure. file not found: %s", app_structure_path.c_str());
				return false;
			}

			mMidiService = mCore->getService<nap::MidiService>();
			mRenderService = mCore->getService<nap::RenderService>();
			mInputService = mCore->getService<nap::InputService>();
			mSDLInputService = mCore->getService<nap::SDLInputService>();
			mGuiService = mCore->getService<nap::IMGuiService>();

			auto pluginType = nap::rtti::TypeInfo::get_by_name(stringPluginClass);
			assert(pluginType.can_create_instance());
			std::vector<rttr::argument>	args = { rttr::argument(*mCore.get()) };
			auto pluginVariant = pluginType.create(args);
			auto plugin = pluginVariant.get_value<nap::AudioPlugin*>();
			assert(plugin != nullptr);
			mPlugin = std::unique_ptr<nap::AudioPlugin>(plugin);

			// Create a callback for parameter registration during the plugin's initialization
			nap::Slot<nap::Parameter&> registerParameterSlot = { this, &NapPluginBridge::registerParameter };

			if (!mPlugin->init(errorState))
			{
				nap::Logger::error("Failed to initialize plugin: %s", errorState.toString().c_str());
				return false;
			}

			mInitialized = true;

			return true;
		}


		tresult PLUGIN_API NapPluginBridge::terminate ()
		{
			if (!mInitialized)
				return kResultOk;
			mInitialized = false;

			if (mTimer)
			{
				mTimer->stop();
				mTimer->release();
			}

			mControlThread.disconnectPeriodicTask(mControlSlot);
			nap::Logger::info("disconnected periodic task");

			std::atomic<bool> napTerminated(false);
			mControlThread.enqueue([&]()
			{
				mPlugin->shutdown();
				mServices = nullptr;
				mCore = nullptr;
				napTerminated = true;
			});
			while (!napTerminated)
				mMainThreadQueue.process();
			mMainThreadQueue.process();

			mControlThread.stop();

			return SingleComponentEffect::terminate ();
		}


 		void NapPluginBridge::processNAPInputEvent(const nap::InputEvent& ev)
 		{
 			std::lock_guard<std::mutex> lock(mMutex);
 			mGuiService->processInputEvent(ev);
 		}


 		void NapPluginBridge::onTimer(Timer *timer)
 		{
			mMainThreadQueue.process();
 		}


		void NapPluginBridge::control(double deltaTime)
		{
			// Begin recording the render commands for the main render window
			std::lock_guard<std::mutex> lock(mMutex);

			std::function<void(double)> updateFunc;
			if (mView != nullptr && mView->isAttached())
				updateFunc = [&](double deltaTime){ mPlugin->update(deltaTime); };
			else
				updateFunc = [](double deltaTime) {};

			mCore->update(updateFunc);

			if (mView != nullptr && mView->isAttached())
				mPlugin->render(mView->getRenderWindow());
			else
				mPlugin->render(nullptr);
		}


		tresult PLUGIN_API NapPluginBridge::process (ProcessData& data)
		{
			// Process parameters
			if (data.inputParameterChanges)
			{
				int32 numParamsChanged = data.inputParameterChanges->getParameterCount ();
				for (int32 index = 0; index < numParamsChanged; index++)
				{
					Vst::IParamValueQueue* paramQueue =
						data.inputParameterChanges->getParameterData (index);
					if (paramQueue)
					{
						Vst::ParamValue value;
						int32 sampleOffset;
						int32 numPoints = paramQueue->getPointCount ();
						if (paramQueue->getParameterId() == kBypassId)
						{
							if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) == kResultTrue)
								mBypass = (value > 0.5f);
						}

						int paramId = 1; // start from 1, 0 is reserved for bypass
						for (auto& parameter : mParameters)
						{
							if (paramQueue->getParameterId() == paramId++)
							{
								if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) == kResultTrue)
								{
									auto floatParam = rtti_cast<nap::ParameterFloat>(parameter);
									if (floatParam != nullptr)
										mControlThread.enqueue([floatParam, value](){
											floatParam->setValue(nap::math::fit<float>(value, 0.f, 1.f, floatParam->mMinimum, floatParam->mMaximum));
										});
									auto intParam = rtti_cast<nap::ParameterInt>(parameter);
									if (intParam != nullptr)
										mControlThread.enqueue([intParam, value](){
											intParam->setValue(nap::math::fit<float>(value, 0.f, 1.f, intParam->mMinimum, intParam->mMaximum));
										});
									auto optionParam = rtti_cast<nap::ParameterDropDown>(parameter);
									if (optionParam != nullptr)
										mControlThread.enqueue([optionParam, value](){
											optionParam->setSelectedIndex(nap::math::fit<float>(value, 0.f, 1.f, 0, optionParam->mItems.size() - 1));
										});
								}
							}
						}
					}
				}
			}

			// Process note events
			auto events = data.inputEvents;
			if (events)
			{
				int32 count = events->getEventCount ();
				for (int32 i = 0; i < count; i++)
				{
					Vst::Event e;
					events->getEvent (i, e);
					switch (e.type)
					{
						case Vst::Event::kNoteOnEvent:
						{
							auto midiEvent = std::make_unique<nap::MidiEvent>(nap::MidiEvent::Type::noteOn, e.noteOn.pitch, e.noteOn.velocity * 127);
							mMidiService->enqueueEvent(std::move(midiEvent));
							break;
						}
						case Vst::Event::kNoteOffEvent:
						{
							auto midiEvent = std::make_unique<nap::MidiEvent>(nap::MidiEvent::Type::noteOff, e.noteOn.pitch, 0);
							mMidiService->enqueueEvent(std::move(midiEvent));
							break;
						}
						default:
							continue;
					}
				}
			}

			// Process audio
			if (data.numOutputs == 0)
			{
				// nothing to do
				return kResultOk;
			}

			if (data.numSamples > 0)
			{
				if (data.numSamples != mAudioService->getNodeManager().getInternalBufferSize())
					mAudioService->getNodeManager().setInternalBufferSize(data.numSamples);

				// Process Algorithm
				mAudioService->onAudioCallback(data.numInputs > 0 ? data.inputs[0].channelBuffers32 : nullptr, data.outputs[0].channelBuffers32, data.numSamples);
			}

			return kResultOk;
		}


		tresult PLUGIN_API NapPluginBridge::setActive (TBool state)
		{
			return kResultOk;
		}


		tresult PLUGIN_API NapPluginBridge::setState (IBStream* state)
		{
			IBStreamer streamer (state, kLittleEndian);
			int32 savedBypass = 0;
			if (!streamer.readInt32 (savedBypass))
				return kResultFalse;
			mBypass = savedBypass > 0;

			return kResultOk;
		}


		tresult PLUGIN_API NapPluginBridge::getState (IBStream* state)
		{
			IBStreamer streamer (state, kLittleEndian);

			streamer.writeInt32 (mBypass ? 1 : 0);

			return kResultOk;
		}


		tresult PLUGIN_API NapPluginBridge::setupProcessing (ProcessSetup& newSetup)
		{
			// called before the process call, always in a disable state (not active)
			// here we keep a trace of the processing mode (offline,...) for example.
			mAudioService->getNodeManager().setSampleRate(newSetup.sampleRate);
			mAudioService->getNodeManager().setInternalBufferSize(newSetup.maxSamplesPerBlock);

			mProcessingMode = newSetup.processMode;
			return SingleComponentEffect::setupProcessing (newSetup);
		}


		tresult PLUGIN_API NapPluginBridge::setBusArrangements (SpeakerArrangement* inputs, int32 numIns,
		                                                    SpeakerArrangement* outputs, int32 numOuts)
		{
			auto& nodeManager = mAudioService->getNodeManager();
			nodeManager.setInputChannelCount(numIns);
			nodeManager.setOutputChannelCount(numOuts);

			SingleComponentEffect::setBusArrangements (inputs, numIns, outputs, numOuts);
			return kResultFalse;
		}


		tresult PLUGIN_API NapPluginBridge::canProcessSampleSize (int32 symbolicSampleSize)
		{
			if (symbolicSampleSize == kSample32)
				return kResultTrue;

			// we support double processing
			if (symbolicSampleSize == kSample64)
				return kResultTrue;

			return kResultFalse;
		}


		IPlugView* PLUGIN_API NapPluginBridge::createView (const char* name)
		{
			if (mView != nullptr)
				return nullptr;

			auto size = mPlugin->getRenderWindowSize();
			ViewRect rect = ViewRect(0, 0, size.x, size.y);
			mView = new NapPluginView(*this, mMainThreadQueue, rect);
			return mView;
		}


		tresult PLUGIN_API NapPluginBridge::setEditorState (IBStream* state)
		{
			return kResultTrue;
		}


		tresult PLUGIN_API NapPluginBridge::getEditorState (IBStream* state)
		{
			return kResultTrue;
		}


		tresult PLUGIN_API NapPluginBridge::setParamNormalized(ParamID tag, ParamValue value)
		{
			// called from host to update our parameters state
			tresult result = SingleComponentEffect::setParamNormalized (tag, value);
			return result;
		}


		tresult PLUGIN_API NapPluginBridge::getParamStringByValue(ParamID tag, ParamValue valueNormalized, String128 string)
		{
			return SingleComponentEffect::getParamStringByValue(tag, valueNormalized, string);
		}


		tresult PLUGIN_API NapPluginBridge::getParamValueByString(ParamID tag, TChar* string, ParamValue& valueNormalized)
		{
			return SingleComponentEffect::getParamValueByString(tag, string, valueNormalized);
		}


		tresult PLUGIN_API NapPluginBridge::queryInterface(const TUID iid, void** obj)
		{
			return SingleComponentEffect::queryInterface(iid, obj);
		}


		void NapPluginBridge::registerParameter(nap::Parameter& napParameter)
		{
			auto paramID = kBypassId + 1;

			Vst::TChar paramName[128];

 			if (napParameter.get_type() == RTTI_OF(nap::ParameterFloat))
			{
     			mParameters.emplace_back(&napParameter);
 				auto napParameterFloat = rtti_cast<nap::ParameterFloat>(&napParameter);
				Steinberg::Vst::StringConvert::convert(napParameterFloat->getDisplayName(), paramName);
				auto parameter = std::make_unique<Vst::RangeParameter>(paramName, paramID++, STR16(""), napParameterFloat->mMinimum, napParameterFloat->mMaximum, napParameterFloat->mValue);
				parameters.addParameter(parameter.release());
			}

			if (napParameter.get_type() == RTTI_OF(nap::ParameterInt))
			{
     			mParameters.emplace_back(&napParameter);
				auto napParameterInt = rtti_cast<nap::ParameterInt>(&napParameter);
				Steinberg::Vst::StringConvert::convert(napParameterInt->getDisplayName(), paramName);
				auto parameter = std::make_unique<Vst::RangeParameter>(paramName, paramID++, STR16(""), napParameterInt->mMinimum, napParameterInt->mMaximum, napParameterInt->mValue, 1.f);
				parameters.addParameter(parameter.release());
			}

			if (napParameter.get_type() == RTTI_OF(nap::ParameterDropDown))
			{
     			mParameters.emplace_back(&napParameter);
				auto napParameterOptionList = rtti_cast<nap::ParameterDropDown>(&napParameter);
				Steinberg::Vst::StringConvert::convert(napParameterOptionList->getDisplayName(), paramName);
				auto parameter = std::make_unique<Vst::StringListParameter>(paramName, paramID++, STR16(""));
				Vst::TChar optionName[128];
				for (auto& option : napParameterOptionList->mItems)
				{
					Steinberg::Vst::StringConvert::convert(option, optionName);
					parameter->appendString(optionName);
				}
				parameters.addParameter(parameter.release());
			}
		}


	} // namespace Vst

} // namespace Steinberg


BEGIN_FACTORY_DEF (stringCompanyName, stringCompanyWeb, stringCompanyEmail)
	//---First plug-in included in this factory-------
	// its kVstAudioEffectClass component
	DEF_CLASS2 (INLINE_UID_FROM_FUID(pluginFUID),
				PClassInfo::kManyInstances,					// cardinality  
				kVstAudioEffectClass,						// the component category (do not change this)
				stringPluginName,							// here the plug-in name (to be changed)
				0,											// single component effects cannot be distributed so this is zero
				stringPluginCategory,						// Subcategory for this plug-in (to be changed)
				stringPluginVersion,						// Plug-in version (to be changed)
				kVstVersionString,							// the VST 3 SDK version (do not change this, always use this define)
				Steinberg::Vst::NapPluginBridge::createInstance) // function pointer called when this component should be instantiated
END_FACTORY

static ModuleInitializer gVSTGUIInit([]{ VSTGUI::init(getPlatformModuleHandle()); });
static ModuleTerminator gVSTGUITerm([]{ VSTGUI::exit(); });

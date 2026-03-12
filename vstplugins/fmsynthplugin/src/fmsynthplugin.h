#pragma once

#include <audioplugin.h>
#include <parametergroup.h>
#include <parametergui.h>
#include <imgui.h>
#include <renderservice.h>
#include <imguiservice.h>
#include <nap/core.h>

namespace nap
{

	class FMSynthPlugin : public AudioPlugin
	{
		RTTI_ENABLE(AudioPlugin)

	public:
		FMSynthPlugin(Core& core) : AudioPlugin(core) { }

		bool init(utility::ErrorState& errorState) override
		{
			mRenderService = getCore().getService<RenderService>();
			mGuiService = getCore().getService<IMGuiService>();

			auto parameterGroup = getCore().getResourceManager()->findObject<nap::ParameterGroup>("Parameters").get();
			if (parameterGroup == nullptr)
			{
				errorState.fail("Can't find parameter group: Parameters");
				return false;
			}

			for (auto& parameter : parameterGroup->mMembers)
				registerParameterSignal(*parameter);

			mParameterGUI = std::make_unique<nap::ParameterGUI>(getCore());
			mParameterGUI->mParameterGroup = parameterGroup;
			if (!mParameterGUI->init(errorState))
			{
				errorState.fail("Failed to initialize parameter GUI.");
				return false;
			}

			return true;
		}

		void update(double deltaTime) override
		{
			ImGui::Begin("NAP", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
			if (mParameterGUI != nullptr)
				mParameterGUI->show(false);
			ImGui::NewLine();
			std::string formattedText = nap::utility::stringFormat("Framerate: %.02f", getCore().getFramerate());
			ImGui::Text(formattedText.c_str());
			ImGui::End();
		}

		void render(nap::RenderWindow* renderWindow) override
		{
			mRenderService->beginFrame();
			if (renderWindow != nullptr)
			{
				if (mRenderService->beginRecording(*renderWindow))
				{
					renderWindow->beginRendering();
					mGuiService->draw();
					renderWindow->endRendering();
					mRenderService->endRecording();
				}
			}
			mRenderService->endFrame();
		}

		void shutdown() override
		{
			mParameterGUI->onDestroy();
			mParameterGUI = nullptr;
		}

	private:
		std::unique_ptr<nap::ParameterGUI> mParameterGUI = nullptr;
		RenderService* mRenderService = nullptr;
		IMGuiService* mGuiService = nullptr;
	};

}
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Std includes
#include <iostream>

// Audio includes
#include "audioservice.h"
#include <audio/core/audionode.h>

// Nap includes
#include <nap/logger.h>

// Third party includes
#ifdef NAP_AUDIOFILE_SUPPORT
    #include <mpg123.h>
#endif

RTTI_BEGIN_CLASS(nap::audio::AudioServiceConfiguration)
	RTTI_PROPERTY("ReserveProcesses", &nap::audio::AudioServiceConfiguration::mReserveProcesses, nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("ReserveRootProcesses", &nap::audio::AudioServiceConfiguration::mReserveRootProcesses, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::audio::AudioService)
	RTTI_CONSTRUCTOR(nap::ServiceConfiguration*)
RTTI_END_CLASS


namespace nap
{
	namespace audio
	{
		AudioService::AudioService(ServiceConfiguration* configuration) : Service(configuration)
		{
			auto config = rtti_cast<AudioServiceConfiguration>(configuration);
			mNodeManager = std::make_unique<NodeManager>(mDeletionQueue, config->mReserveProcesses, config->mReserveRootProcesses);
		}


		AudioService::~AudioService()
		{
		}


		bool AudioService::init(nap::utility::ErrorState& errorState)
		{
#ifdef NAP_AUDIOFILE_SUPPORT
            // Initialize mpg123 library
			mpg123_init();
			mMpg123Initialized = true;
#endif
			checkLockfreeTypes();

			// Start garbage collector
			mGarbageCollectorThread = std::thread([this](){ garbageCollectorLoop(); });

			return true;
		}


		void AudioService::shutdown()
		{
			mDeletionQueue.clear();

#ifdef NAP_AUDIOFILE_SUPPORT
			// Close mpg123 library
			if (mMpg123Initialized)
				mpg123_exit();
#endif

			mStopGarbageCollector.store(true);
			mGarbageCollectorThread.join();
		}


		NodeManager& AudioService::getNodeManager()
		{
			return *mNodeManager;
		}


		void AudioService::onAudioCallback(float** inputBuffer, float** outputBuffer, unsigned long framesPerBuffer)
		{
			// process the node manager
			mNodeManager->process(inputBuffer, outputBuffer, framesPerBuffer);

			// Move objects from the deletion queue to the trash bin.
			// Execute audioCleanup() method for audio::SafeObject descendants.
			auto deletedData = std::move(mDeletionQueue.try_dequeue());
			while (deletedData != nullptr)
			{
				auto object = deletedData->getSafeObject();
				if (object != nullptr)
					object->audioCleanup();
				mTrashBin.enqueue(std::move(deletedData));
				deletedData = std::move(mDeletionQueue.try_dequeue());
			}
		}
		

		void AudioService::checkLockfreeTypes()
		{
            enum EnumType { a, b, c };
            std::atomic<bool> boolVar;
            std::atomic<int> intVar;
            std::atomic<float> floatVar;
            std::atomic<double> doubleVar;
            std::atomic<long> longVar;
            std::atomic<long double> longDoubleVar;
            std::atomic<EnumType> enumVar;

            if (!boolVar.is_lock_free())
                Logger::warn("%s is not lockfree on current platform", "atomic<bool>");
            if (!intVar.is_lock_free())
                Logger::warn("%s is not lockfree on current platform", "atomic<int>");
            if (!floatVar.is_lock_free())
                Logger::warn("%s is not lockfree on current platform", "atomic<float>");
            if (!doubleVar.is_lock_free())
                Logger::warn("%s is not lockfree on current platform", "atomic<double>");
            if (!longVar.is_lock_free())
                Logger::warn("%s is not lockfree on current platform", "atomic<long>");
            if (!longDoubleVar.is_lock_free())
                Logger::warn("%s is not lockfree on current platform", "atomic<long double>");
            if (!enumVar.is_lock_free())
                Logger::warn("%s is not lockfree on current platform", "atomic enum");
		}


		void AudioService::garbageCollectorLoop()
		{
			while (!mStopGarbageCollector.load())
			{
				auto result = std::move(mTrashBin.wait_dequeue(100000));
				result = nullptr;
			}

			while (mTrashBin.try_dequeue());
		}

	}
}

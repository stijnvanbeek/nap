/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "process.h"

#include <audio/core/audionodemanager.h>

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::audio::Process)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::audio::ParentProcess)
RTTI_END_CLASS

namespace nap
{
	namespace audio
	{
		// --- Process  ---//

		Process::Process(NodeManager& nodeManager) : SafeObject(), mNodeManager(&nodeManager)
		{
		}


		Process::Process(ParentProcess& parent) : mNodeManager(&parent.getNodeManager())
		{
		}


		Process::~Process()
		{
			if (mRegisteredWithNodeManager.load())
				getNodeManager().unregisterProcess(*this);
		}


		void Process::audioCleanup()
		{
			// Unregister as root process, if needed
			auto it = std::find_if(getNodeManager().mRootProcesses.begin(), getNodeManager().mRootProcesses.end(), [&](auto& e){ return e.get() == this; });
			if (it != getNodeManager().mRootProcesses.end())
				getNodeManager().mRootProcesses.erase(it);
		}


		void Process::update()
		{
			if (mLastCalculatedSample < getSampleTime())
			{
				mLastCalculatedSample = getSampleTime();
				process();
			}
		}
		
		
		int Process::getBufferSize() const
		{
			return getNodeManager().getInternalBufferSize();
		}
		
		
		float Process::getSampleRate() const
		{
			return getNodeManager().getSampleRate();
		}
		
		
		DiscreteTimeValue Process::getSampleTime() const
		{
			return getNodeManager().getSampleTime();
		}
		
		
		// --- ParentProcess --- //


		ParentProcess::ParentProcess(NodeManager &nodeManager, ThreadPool &threadPool, int reserveChildren): Process(nodeManager), mThreadPool(threadPool)
		{
			mChildren.reserve(reserveChildren);
		}


		ParentProcess::ParentProcess(ParentProcess &parent, int reserveChildren): Process(parent), mThreadPool(parent.mThreadPool)
		{
			mChildren.reserve(reserveChildren);
		}


		void ParentProcess::addChild(Process& child)
		{
			auto childPtr = child.getSafe();
			auto parentPtr = getSafe();
			getNodeManager().enqueueTask([&, parentPtr, childPtr]() {
				// Check if parent and child are still valid
				if (parentPtr == nullptr || childPtr == nullptr)
					return;

				// Check for duplicates
				auto it = std::find_if(mChildren.begin(), mChildren.end(), [&](auto& e){ return e.get() == childPtr.get(); });
				if (it == mChildren.end())
					mChildren.emplace(childPtr);
				sortChildrenByThread();
			});
		}
		
		
		void ParentProcess::removeChild(Process& child)
		{
			auto childPtr = child.getSafe();
			auto parentPtr = getSafe();
			getNodeManager().enqueueTask([&, parentPtr, childPtr]() {
				// Check if parent and child are still valid
				if (parentPtr == nullptr || childPtr == nullptr)
					return;

				mChildren.erase(childPtr);
				sortChildrenByThread();
			});
		}
		
		
		void ParentProcess::processSequential()
		{
			for (auto& child : mChildren)
			{
				if (child != nullptr) // Check if the child is not enqueued for deletion in the meantime
					child->update();
			}
		}


		void ParentProcess::sortChildrenByThread()
		{
			auto audioThreadCount = std::max<int>(mThreadPool.getThreadCount(), 1); // There is always at least one (device) audio thread.
			auto parallelCount = std::min<int>(audioThreadCount, mChildren.size());
			mThreadData.clear();
			for (auto i = 0; i < parallelCount; ++i)
				mThreadData.emplace(std::make_unique<ThreadData>());
			auto it_children = mChildren.begin();
			auto it_threadData = mThreadData.begin();
			while (it_children != mChildren.end())
			{
				(*it_threadData)->mChildren.emplace(*it_children);
				it_children++;
				it_threadData++;
				if (it_threadData == mThreadData.end())
					it_threadData = mThreadData.begin();
			}
		}


		void ParentProcess::processParallel()
		{
			auto parallelCount = std::min<int>(mThreadPool.getThreadCount(), mChildren.size());
			if (parallelCount != mThreadData.size())
				sortChildrenByThread();

			for (auto& threadData : mThreadData)
				threadData->mFinished.store(false);

			auto first = mThreadData.begin();
			for (auto& threadData : mThreadData)
			{
				if (threadData.get() == first->get())
					continue;
				auto threadDataPtr = threadData.get();
				mThreadPool.execute([&, threadDataPtr]() {
					for (auto& child : threadDataPtr->mChildren)
						if (child->getSafe() != nullptr)
							child->update();
					threadDataPtr->mFinished.store(true);
				});
			}

			if (!mThreadData.empty())
			{
				for (auto& child : (*first)->mChildren)
					if (child->getSafe() != nullptr)
						child->update();
				(*first)->mFinished.store(true);
			}

			bool finished = false;
			while (!finished)
			{
				finished = true;
				for (auto& threadData : mThreadData)
					if (!threadData->mFinished.load())
						finished = false;
			}
		}
		
		
		void ParentProcess::process()
		{
			// First remove children that are (being) deleted or enqueued for deletion
			bool childRemoved = false;
			auto it = mChildren.begin();
			while (it != mChildren.end())
			{
				if (*it == nullptr)
				{
					mChildren.erase(it);
					childRemoved = true;
					it = mChildren.begin(); // The unordered_set is shuffled after erasing, so we start again.
				}
				else
					it++;
			}
			if (childRemoved)
				sortChildrenByThread();

			switch (mMode)
			{
				case Mode::Sequential:
					processSequential();
					break;
				case Mode::Parallel:
					processParallel();
					break;
			}
		}
		
	}
	
}

#include "threading.h"

namespace nap
{
    
    TaskQueue::TaskQueue(unsigned int maxQueueItems) : mQueue(maxQueueItems)
    {
        mDequeuedTasks.resize(maxQueueItems);
    }
    
    
    
    void TaskQueue::processBlocking()
    {
        auto it = mDequeuedTasks.begin();
        auto count = mQueue.wait_dequeue_bulk(it, mDequeuedTasks.size());
        for (auto i = 0; i < count; ++i)
            (*it++)();
    }
    
    
    void TaskQueue::process()
    {
        auto it = mDequeuedTasks.begin();
        auto count = mQueue.try_dequeue_bulk(it, mDequeuedTasks.size());
        for (auto i = 0; i < count; ++i)
            (*it++)();
    }
    
    
    WorkerThread::WorkerThread(bool blocking, unsigned int maxQueueItems) : mBlocking(blocking), mTaskQueue(maxQueueItems)
    {
        mRunning = false;
    }
    
    
    WorkerThread::~WorkerThread()
    {
        stop();
    }
    
    
    void WorkerThread::start()
    {
        if (mRunning)
            return;
        
        mRunning = true;
        
        if (mBlocking)
        {
            mThread = std::make_unique<std::thread>([&](){
                while (mRunning)
                    mTaskQueue.processBlocking();
            });
        }
        else {
            mThread = std::make_unique<std::thread>([&](){
                while (mRunning) {
                    mTaskQueue.process();
                    loop();
                }
            });
        }
    }
    
    
    void WorkerThread::stop()
    {
        if (!mRunning)
            return;
        
        mRunning = false;
        
        // enqueue empty function for thread to make it stop blocking
        mTaskQueue.enqueue([](){});
        
        mThread->join();
    }
    
    
    ThreadPool::ThreadPool(unsigned int numberOfThreads, unsigned int maxQueueItems)
        : mTaskQueue(maxQueueItems)
    {
        mStop = false;
        for (unsigned int i = 0; i < numberOfThreads; ++i)
            addThread();
    }
    
    
    ThreadPool::~ThreadPool()
    {
        shutDown();
    }
    
    
    void ThreadPool::shutDown()
    {
        mStop = true;
        
        // enqueue empty function for each thread to make it stop blocking
        for (unsigned int i = 0; i < mThreads.size(); ++i)
            mTaskQueue.enqueue([](){});
        
        for (auto& thread : mThreads)
            thread.join();
        
        mThreads.clear();
    }
    
    
    void ThreadPool::resize(int numberOfThreads)
    {
        shutDown();
        
        mStop = false;
        for (auto i = 0; i < numberOfThreads; ++i)
            addThread();
    }
    
    
    void ThreadPool::addThread()
    {
        mThreads.emplace_back([&](){
            while (!mStop)
                mTaskQueue.processBlocking();
        });
    }
    
    
}
//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "ThreadManager.h"
#include "Environment.h"
#include "Reflection.h"
#include "Profiler.h"

namespace tri {

	TRI_SYSTEM_INSTANCE(ThreadManager, env->threadManager);

	void ThreadManager::init() {
		threadMutex = std::make_shared<std::mutex>();
		taskMutex = std::make_shared<std::mutex>();
		taskCondition = std::make_shared<std::condition_variable>();
	}

	void ThreadManager::startup() {
		for (int i = 0; i < workerThreadCount; i++) {
			auto worker = std::make_shared<Worker>();
			workers.push_back(worker);
			worker->workerId = i;
			worker->threadManager = this;
			worker->run();
		}
	}

	void ThreadManager::shutdown() {
		for (int i = 0; i < threads.size(); i++) {
			auto& thread = threads[i];
			if (thread.thread) {
				if (thread.thread->joinable()) {
					thread.thread->detach();
				}
				thread.thread = nullptr;
			}
		}
		threads.clear();
		workers.clear();
		tasks.clear();
		taskMutex = nullptr;
		taskCondition = nullptr;
	}

	int ThreadManager::addThread(const std::string& name, const std::function<void()>& callback) {
		std::unique_lock<std::mutex> lock(*threadMutex);
		Thread thread;
		thread.threadId = nextThreadId++;
		thread.name = name;
		thread.thread = std::make_shared<std::thread>([name, callback]() {
			TRI_PROFILE_THREAD(name.c_str());
			callback();
		});
		threads.push_back(thread);
		return thread.threadId;
	}

	void ThreadManager::joinThread(int threadId) {
		for (auto& thread : threads) {
			if (thread.threadId == threadId) {
				if (thread.thread) {
					if (thread.thread->joinable()) {
						thread.thread->join();
					}
				}
				break;
			}
		}
	}

	void ThreadManager::terminateThread(int threadId) {
		for (int i = 0; i < threads.size(); i++) {
			auto& thread = threads[i];
			if (thread.threadId == threadId) {
				if (thread.thread) {
					if (thread.thread->joinable()) {
						thread.thread->detach();
					}
					thread.thread = nullptr;
					threads.erase(threads.begin() + i);
				}
				break;
			}
		}
	}

	int ThreadManager::addTask(const std::function<void()>& callback) {
		Task task;
		task.taskId = nextTaskId++;
		task.isWorkedOn = false;
		task.callback = callback;

		taskMutex->lock();
		tasks.push_back(task);
		taskMutex->unlock();

		for (auto& worker : workers) {
			if (worker) {
				if (worker->isWaiting) {
					worker->condition.notify_one();
					worker->isWaiting = false;
					break;
				}
			}
		}

		return task.taskId;
	}

	void ThreadManager::joinTask(int taskId) {
		std::unique_lock<std::mutex> lock(*taskMutex);
		bool found = true;
		while (found) {
			found = false;
			for (int i = 0; i < tasks.size(); i++) {
				if (tasks[i].taskId == taskId) {
					found = true;
					taskCondition->wait(lock);
				}
			}
		}
	}

	bool ThreadManager::isTaskFinished(int taskId) {
		std::unique_lock<std::mutex> lock(*taskMutex);
		for (int i = 0; i < tasks.size(); i++) {
			if (tasks[i].taskId == taskId) {
				return false;
			}
		}
		return true;
	}

	void ThreadManager::Worker::run() {
		running = true;
		taskId = -1;
		threadId = threadManager->addThread(std::string("Worker Thread ") + std::to_string(workerId), [this]() {
			while (running) {
				threadManager->taskMutex->lock();
				if (!threadManager->tasks.empty()) {

					std::function<void()> callback;
					for (int i = 0; i < threadManager->tasks.size(); i++) {
						auto& task = threadManager->tasks[i];
						if (!task.isWorkedOn) {
							taskId = task.taskId;
							callback = task.callback;
							task.isWorkedOn = true;
							break;
						}
					}

					threadManager->taskMutex->unlock();
					if (callback) {
						TRI_PROFILE("task");
						callback();
					}
					threadManager->taskMutex->lock();
					for (int i = 0; i < threadManager->tasks.size(); i++) {
						if (threadManager->tasks[i].taskId == taskId) {
							threadManager->tasks.erase(threadManager->tasks.begin() + i);
							break;
						}
					}
					taskId = -1;
					threadManager->taskCondition->notify_one();
					threadManager->taskMutex->unlock();
				}
				else {
					threadManager->taskMutex->unlock();
					std::unique_lock<std::mutex> lock(mutex);
					isWaiting = true;
					condition.wait(lock);
					isWaiting = false;
				}
			}
		});
	}

}

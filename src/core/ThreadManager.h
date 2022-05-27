//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "System.h"

namespace tri {

	class ThreadManager : public System {
	public:
		int workerThreadCount = std::thread::hardware_concurrency();

		void startup() override;
		void shutdown() override;

		int addThread(const std::string& name, const std::function<void()>& callback);
		void joinThread(int threadId);
		void terminateThread(int threadId);

		int addTask(const std::function<void()>& callback);
		void joinTask(int taskId);
		bool isTaskFinished(int taskId);

	private:
		int nextThreadId = 0;
		int nextTaskId = 0;

		class Thread {
		public:
			std::shared_ptr<std::thread> thread;
			std::string name;
			int threadId;
		};
		std::vector<Thread> threads;

		class Task {
		public:
			std::function<void()> callback;
			int taskId;
			bool isWorkedOn;
		};
		std::vector<Task> tasks;
		std::shared_ptr<std::mutex> taskMutex;
		std::shared_ptr<std::condition_variable> taskCondition;

		class Worker {
		public:
			int workerId;
			int threadId;
			int taskId;
			bool running;
			ThreadManager* threadManager;
			std::mutex mutex;
			std::condition_variable condition;
			bool isWaiting = false;
			void run();
		};
		friend class Worker;
		std::vector<std::shared_ptr<Worker>> workers;
	};

}

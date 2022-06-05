//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "System.h"
#include "Reflection.h"
#include <barrier>

namespace tri {

	class JobManager : public System {
	public:
		bool enableMultithreading = true;

		void init() override;
		void shutdown() override;

		class Job {
		public:
			std::string name;
			bool enableMultithreading;

			template<typename T>
			void addSystem() {
				addSystem(Reflection::getDescriptor<T>()->name);
			}
			template<typename... T>
			void addSystems() {
				addSystems({ Reflection::getDescriptor<T>()->name... });
			}
			void addSystem(const std::string& name);
			void addSystems(const std::vector<std::string>& names);

			void removeSystem(const std::string& name);
			void orderSystems(const std::vector<std::string>& systems);
			void addJobExclusion(const std::string& name);

		private:
			friend class JobManager;
			std::vector<std::string> systemNames;
			std::vector<int> systemClassIds;
			std::vector<std::string> jobExclusion;
			std::vector<std::vector<std::string>> orderConstraints;
			void sort();
		};

		Job* addJob(const std::string& name, const std::vector<std::string>& systems = {});
		Job* getJob(const std::string& name);
		void removeJob(const std::string& name);

		void startupJobs();
		void tickJobs();
		void startupPendingSystems(bool invokeEvent);
		void shutdownPendingSystems(bool invokeEvent);
		void shutdownJobs();

	private:

		class JobHandle {
		public:
			Job job;
			int threadId;
			JobManager* jobManager;
			std::mutex mutex;
			bool isDefaultJob;
			bool pendingRemove = false;
			bool isThreadFinished = false;

			void run();
			void tick();
			void startupSystems();
			void shutdownSystems();
		};
		friend class JobHandle;
		std::vector<std::shared_ptr<JobHandle>> jobs;

		std::atomic<bool> startupSystems;
		std::atomic<bool> shutdownSystems;
		std::atomic<bool> updateSyncBarrier;
		std::atomic<bool> running;
		int runningJobCount;

		std::shared_ptr<std::barrier<>> syncBarrierBegin;
		std::shared_ptr<std::barrier<>> syncBarrierEnd;
		std::shared_ptr<std::barrier<>> syncBarrierUpdate;
		int syncBarrierSize;

		JobHandle* getJobHandle(const std::string& name);
		JobHandle* getDefaultJob();
		void syncBegin();
		void syncEnd();
	};

}

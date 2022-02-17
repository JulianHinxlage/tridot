//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "Environment.h"
#include "System.h"
#include "util/Ref.h"
#include <barrier>

namespace tri {

	class JobSystem : public System {
	public:
		bool enableMultiThreading;

		JobSystem();

		void startup() override;
		void update() override;
		void shutdown() override;

		void addJob(const std::string& jobName, const std::vector<std::string> &updateCallbacks);
		void addJobExclusion(const std::string& jobName, const std::vector<std::string>& exclusions);
		void setJobThreading(const std::string& jobName, bool multiThreadingEnabled);

	private:
		class Job {
		public:
			std::string name;
			std::vector<std::string> updateCallbacks;
			std::vector<std::string> jobExclusions;
			bool useSingleThreading;
			int threadId;
			bool isDefaultJob;
			bool initFlag;
			std::mutex mutex;
			JobSystem* system;

			Job(const std::string& name, const std::vector<std::string>& updateCallbacks) : name(name), updateCallbacks(updateCallbacks), useSingleThreading(false), threadId(-1), isDefaultJob(false), initFlag(false) {}
			void update();
		};

		std::vector<Ref<Job>> jobs;
		std::atomic_bool running;
		Ref<std::barrier<>> startBarrier;
		Ref<std::barrier<>> endBarrier;
		Ref<std::barrier<>> oldStartBarrier;
		int barrierSize;

		void initNewJobs();
		Job *getJob(const std::string &jobName);
	};

}

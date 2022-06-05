//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "JobManager.h"
#include "Environment.h"
#include "ThreadManager.h"
#include "Profiler.h"
#include "Console.h"
#include "CrashHandler.h"
#include "EventManager.h"

#include <csetjmp>

namespace tri {

	TRI_SYSTEM_INSTANCE(JobManager, env->jobManager);

	void JobManager::init() {
		runningJobCount = 0;
		syncBarrierSize = 0;
		running = false;
		startupSystems = false;
		shutdownSystems = false;
		updateSyncBarrier = false;
		getDefaultJob();
	}

	void JobManager::shutdown() {
		
	}

	JobManager::Job* JobManager::addJob(const std::string& name, const std::vector<std::string>& systems) {
		if(auto *job = getJobHandle(name)) {
			if (!job->pendingRemove) {
				for (auto& system : systems) {
					job->job.addSystem(system);
				}
				return &job->job;
			}
		}
		auto handle = std::make_shared<JobHandle>();
		handle->threadId = -1;
		handle->jobManager = this;
		handle->job.name = name;
		handle->job.enableMultithreading = true;
		handle->job.systemNames = systems;
		handle->isDefaultJob = false;
		handle->pendingRemove = false;
		jobs.push_back(handle);
		return &handle->job;
	}

	JobManager::Job* JobManager::getJob(const std::string& name) {
		for (auto& handle : jobs) {
			if (handle->job.name == name) {
				return &handle->job;
			}
		}
		return nullptr;
		//auto handle = std::make_shared<JobHandle>();
		//handle->threadId = -1;
		//handle->jobManager = this;
		//handle->job.name = name;
		//handle->job.enableMultithreading = true;
		//handle->isDefaultJob = false;
		//handle->pendingRemove = false;
		//jobs.push_back(handle);
		//return &handle->job;
	}

	void JobManager::removeJob(const std::string& name) {
		for (int i = 0; i < jobs.size(); i++) {
			auto& handle = jobs[i];
			if (handle->job.name == name) {
				handle->pendingRemove = true;
				return;
			}
		}
	}

	JobManager::JobHandle* JobManager::getJobHandle(const std::string& name) {
		for (auto& handle : jobs) {
			if (handle->job.name == name) {
				return handle.get();
			}
		}
		return nullptr;
	}

	JobManager::JobHandle* JobManager::getDefaultJob() {
		for (auto& handle : jobs) {
			if (handle->isDefaultJob) {
				return handle.get();
			}
		}
		addJob("Default");
		auto* defaultJob = getJobHandle("Default");
		defaultJob->isDefaultJob = true;
		return defaultJob;
	}



	void JobManager::Job::addSystem(const std::string& name) {
		for (int i = 0; i < systemNames.size(); i++) {
			if (systemNames[i] == name) {
				return;
			}
		}
		systemNames.push_back(name);
		sort();
	}

	void JobManager::Job::addSystems(const std::vector<std::string>& names) {
		for (auto& name : names) {
			addSystem(name);
		}
	}

	void JobManager::Job::removeSystem(const std::string& name) {
		for (int i = 0; i < systemNames.size(); i++) {
			if (systemNames[i] == name) {
				systemNames.erase(systemNames.begin() + i);
				return;
			}
		}
	}

	void JobManager::Job::orderSystems(const std::vector<std::string>& systems) {
		orderConstraints.push_back(systems);
		sort();
	}

	void JobManager::Job::addJobExclusion(const std::string& name) {
		for (int i = 0; i < jobExclusion.size(); i++) {
			if (jobExclusion[i] == name) {
				return;
			}
		}
		jobExclusion.push_back(name);
	}

	void JobManager::Job::sort() {
		for (auto& order : orderConstraints) {
			std::string prev;
			int prevIndex = -1;
			for (int i = 0; i < order.size(); i++) {
				std::string system = order[i];
				int index = -1;

				for (int j = 0; j < systemNames.size(); j++) {
					if (systemNames[j] == system) {
						index = j;
						break;
					}
				}

				if (i > 0 && index != -1 && prevIndex != -1) {
					if (prevIndex > index) {
						systemNames.erase(systemNames.begin() + prevIndex);
						systemNames.insert(systemNames.begin() + index, prev);
						prevIndex = index;
						index++;
					}
				}

				prev = system;
				prevIndex = index;
			}
		}
	}



	void JobManager::startupJobs() {
		running = true;

		if (env->systemManager->hasPendingStartups()) {
			//setup default job
			auto* defualtJob = getDefaultJob();
			defualtJob->job.systemNames.clear();
			for (auto* desc : Reflection::getDescriptors()) {
				if (desc && (desc->flags & ClassDescriptor::SYSTEM)) {
					if (auto* sys = env->systemManager->getSystem(desc->classId)) {

						bool assigedToJob = false;
						for (int i = 0; i < jobs.size(); i++) {
							auto& job = jobs[i];
							if (job) {
								for (auto& name : job->job.systemNames) {
									if (name == desc->name) {
										assigedToJob = true;
										break;
									}
								}
								if (assigedToJob) {
									break;
								}
							}
						}

						if (!assigedToJob) {
							defualtJob->job.systemNames.push_back(desc->name);
						}

					}
				}
			}
		}

		if (!enableMultithreading) {
			return;
		}

		bool updateBarriers = false;
		//count removed jobs
		for (auto& handle : jobs) {
			if (!handle->isDefaultJob) {
				if (handle->job.enableMultithreading) {
					if (handle->pendingRemove && !handle->isThreadFinished) {
						runningJobCount--;
						updateBarriers = true;
					}
				}
			}
		}

		//remove jobs
		for (int i = 0; i < jobs.size(); i++) {
			if (jobs[i]->isThreadFinished || (!jobs[i]->job.enableMultithreading && jobs[i]->pendingRemove)) {
				env->threadManager->joinThread(jobs[i]->threadId);
				env->threadManager->terminateThread(jobs[i]->threadId);
				jobs.erase(jobs.begin() + i);
				i--;
			}
		}

		//count new jobs
		for (auto& handle : jobs) {
			if (handle->threadId == -1) {
				if (!handle->isDefaultJob) {
					if (handle->job.enableMultithreading) {
						runningJobCount++;
						updateBarriers = true;
					}
				}
			}
		}

		//update barriers to new job count
		if (runningJobCount + 1 != syncBarrierSize || updateBarriers) {
			if (syncBarrierSize != 0) {
				syncBarrierUpdate = std::make_shared<std::barrier<>>(syncBarrierSize);
				updateSyncBarrier = true;
				syncBegin();
				syncBarrierUpdate->arrive_and_wait();
			}
			syncBarrierBegin = std::make_shared<std::barrier<>>(runningJobCount + 1);
			syncBarrierEnd = std::make_shared<std::barrier<>>(runningJobCount + 1);
			if (updateSyncBarrier) {
				updateSyncBarrier = false;
				syncBarrierUpdate->arrive_and_wait();
			}
			syncBarrierSize = runningJobCount + 1;
		}

		//launch threads of new jobs
		for (auto& handle : jobs) {
			if (handle->threadId == -1) {
				if (!handle->isDefaultJob) {
					if (handle->job.enableMultithreading) {
						handle->run();
					}
				}
			}
		}
	}

	void JobManager::tickJobs() {
		startupJobs(); // for new jobs

		syncBegin();
		getDefaultJob()->tick();
		syncEnd();

		//single threading jobs
		for (auto& handle : jobs) {
			if (!handle->isDefaultJob) {
				if (!handle->job.enableMultithreading || !enableMultithreading) {
					handle->tick();
				}
			}
		}
	}

	void JobManager::startupPendingSystems(bool invokeEvent) {
		startupJobs();

		startupSystems = true;
		syncBegin();
		getDefaultJob()->startupSystems();
		if (invokeEvent) {
			env->eventManager->startup.invoke();
		}
		syncEnd();

		//single threading jobs
		for (auto& handle : jobs) {
			if (!handle->isDefaultJob) {
				if (!handle->job.enableMultithreading || !enableMultithreading) {
					handle->startupSystems();
				}
			}
		}

		startupSystems = false;
	}

	void JobManager::shutdownPendingSystems(bool invokeEvent) {
		shutdownSystems = true;
		syncBegin();
		getDefaultJob()->shutdownSystems();
		if (invokeEvent) {
			env->eventManager->shutdown.invoke();
		}
		syncEnd();

		//single threading jobs
		for (auto& handle : jobs) {
			if (!handle->isDefaultJob) {
				if (!handle->job.enableMultithreading || !enableMultithreading) {
					handle->shutdownSystems();
				}
			}
		}

		shutdownSystems = false;
	}

	void JobManager::shutdownJobs() {
		if (running) {
			running = false;
			syncBegin();
			syncEnd();

			for (auto& handle : jobs) {
				if (handle->threadId != -1) {
					env->threadManager->joinThread(handle->threadId);
					env->threadManager->terminateThread(handle->threadId);
					handle->threadId = -1;
				}
			}
			runningJobCount = 0;
		}
	}

	void JobManager::syncBegin() {
		if (syncBarrierBegin) {
			syncBarrierBegin->arrive_and_wait();
		}
	}

	void JobManager::syncEnd() {
		if (syncBarrierEnd) {
			syncBarrierEnd->arrive_and_wait();
		}
	}



	void JobManager::JobHandle::run() {
		threadId = env->threadManager->addThread(job.name, [&]() {
			isThreadFinished = false;
			while (true) {
				jobManager->syncBegin();
				if (!jobManager->running) {
					jobManager->syncEnd();
					break;
				}
				else if (jobManager->startupSystems) {
					startupSystems();
				}
				else if (jobManager->shutdownSystems) {
					shutdownSystems();
				}
				else if (!jobManager->updateSyncBarrier) {
					tick();
				}
				else {
					jobManager->syncBarrierUpdate->arrive_and_wait();
					jobManager->syncBarrierUpdate->arrive_and_wait();
					if (pendingRemove) {
						break;
					}
					continue;
				}
				jobManager->syncEnd();
			}
			isThreadFinished = true;
		});
	}

	void JobManager::JobHandle::tick() {

		mutex.lock();
		for (auto& name : job.jobExclusion) {
			auto *handle = jobManager->getJobHandle(name);
			if (handle && handle != this) {
				handle->mutex.lock();
			}
		}

		TRI_PROFILE_FUNC();
		env->profiler->begin(job.name.c_str());

		int recoveryIndex = 0;

		//recovery point: if an unhandled exception occurred the execution continuous at this point
		//in that case the next system after the one, that caused the exception, should be continued with
		int recovery = setjmp(*(jmp_buf*)env->systemManager->getSystem<CrashHandler>()->recoveryPoint);
		if (recovery) {
			env->console->info("crash recovery performed");
			recoveryIndex++;
		}

		int index = 0;
		for (auto& name : job.systemNames) {
			if (index >= recoveryIndex) {
				if (auto* desc = Reflection::getDescriptor(name)) {
					if (auto* sys = env->systemManager->getSystem(desc->classId)) {
						auto* handle = env->systemManager->getSystemHandle(desc->classId);
						if (handle->active) {
							TRI_PROFILE_NAME(desc->name.c_str(), desc->name.size());
							env->profiler->begin(desc->name.c_str());
							sys->tick();
							env->profiler->end();
						}
					}
				}
				recoveryIndex++;
			}
			index++;
		}

		//invoke the tick event on the default job
		if (isDefaultJob && recoveryIndex <= job.systemNames.size()) {
			env->eventManager->tick.invoke();
		}

		env->profiler->end();

		mutex.unlock();
		for (auto& name : job.jobExclusion) {
			auto* handle = jobManager->getJobHandle(name);
			if (handle && handle != this) {
				handle->mutex.unlock();
			}
		}
	}

	void JobManager::JobHandle::startupSystems() {
		TRI_PROFILE_FUNC();
		int recoveryIndex = 0;

		//recovery point: if an unhandled exception occurred the execution continuous at this point
		//in that case the next system after the one, that caused the exception, should be continued with
		int recovery = setjmp(*(jmp_buf*)env->systemManager->getSystem<CrashHandler>()->recoveryPoint);
		if (recovery) {
			env->console->info("crash recovery performed");
			recoveryIndex++;
		}

		int index = 0;
		for (auto& name : job.systemNames) {
			if (index >= recoveryIndex) {
				if (auto* desc = Reflection::getDescriptor(name)) {
					if (auto* sys = env->systemManager->getSystem(desc->classId)) {
						auto* handle = env->systemManager->getSystemHandle(desc->classId);
						if (!handle->wasStartup) {
							TRI_PROFILE_NAME(desc->name.c_str(), desc->name.size());
							sys->startup();
							handle->wasStartup = true;
						}
					}
				}
				recoveryIndex++;
			}
			index++;
		}
	}

	void JobManager::JobHandle::shutdownSystems() {
		TRI_PROFILE_FUNC();
		int recoveryIndex = 0;

		//recovery point: if an unhandled exception occurred the execution continuous at this point
		//in that case the next system after the one, that caused the exception, should be continued with
		int recovery = setjmp(*(jmp_buf*)env->systemManager->getSystem<CrashHandler>()->recoveryPoint);
		if (recovery) {
			env->console->info("crash recovery performed");
			recoveryIndex++;
		}

		int index = 0;
		for (auto& name : job.systemNames) {
			if (index >= recoveryIndex) {
				if (auto* desc = Reflection::getDescriptor(name)) {
					if (auto* sys = env->systemManager->getSystem(desc->classId)) {
						auto* handle = env->systemManager->getSystemHandle(desc->classId);
						if (!handle->wasShutdown && handle->pendingShutdown) {
							TRI_PROFILE_NAME(desc->name.c_str(), desc->name.size());
							sys->shutdown();
							handle->wasShutdown = true;
						}
					}
				}
				recoveryIndex++;
			}
			index++;
		}
	}

}

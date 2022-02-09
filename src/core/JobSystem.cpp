//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "JobSystem.h"
#include "core.h"

namespace tri {

	TRI_REGISTER_SYSTEM_INSTANCE(JobSystem, env->jobSystem);

	JobSystem::JobSystem() {
		enableMultiThreading = false;
		running = false;
		barrierSize = 0;
	}

	void JobSystem::startup() {
		addJob("Default", {});
		jobs.back()->isDefaultJob = true;

		addJob("Hierarchy", { "HierarchySystem" });
		addJob("Window and GUI", { "Gui end", "Window", "Gui begin", "Gizmos begin", "Editor"});
		addJob("Physics", { "Physics" });
		addJob("Render Submit", { "Camera", "Skybox", "MeshComponent", "Renderer" });

		initNewJobs();
	}

	void JobSystem::update() {
		TRI_PROFILE("JobSystem");

		initNewJobs();

		//set default job update callbacks
		for (auto defaultJob : jobs) {
			if (defaultJob && defaultJob->isDefaultJob) {
				defaultJob->updateCallbacks.clear();
				defaultJob->updateCallbacks.push_back("JobSystem");
				defaultJob->updateCallbacks.push_back("RenderThread");
				defaultJob->updateCallbacks.push_back("Scene");
				for (auto job : jobs) {
					if (job && !job->isDefaultJob) {
						for (auto& update : job->updateCallbacks) {
							defaultJob->updateCallbacks.push_back(update);
						}
					}
				}
			}
		}

		if (enableMultiThreading) {
			if (oldStartBarrier) {
				oldStartBarrier->arrive_and_wait();
				{
					ZoneScopedNC("waitForJobs", tracy::Color::Gray30);
					endBarrier->arrive_and_wait();
				}
				oldStartBarrier = nullptr;
				endBarrier = Ref<std::barrier<>>::make(jobs.size() + 1);
			}
			else {
				startBarrier->arrive_and_wait();
				{
					ZoneScopedNC("waitForJobs", tracy::Color::Gray30);
					endBarrier->arrive_and_wait();
				}
			}

		}
		else {
			for (auto job : jobs) {
				if (job) {
					job->update();
				}
			}
		}
	}

	void JobSystem::shutdown() {
		running = false;
		if (enableMultiThreading) {
			startBarrier->arrive_and_wait();
			endBarrier->arrive_and_wait();
			for (auto job : jobs) {
				if (job && job->threadId != -1) {
					env->threads->jointThread(job->threadId);
				}
			}
		}
		jobs.clear();
	}

	void JobSystem::addJob(const std::string& jobName, const std::vector<std::string>& updateCallbacks) {
		if (auto* job = getJob(jobName)) {
			job->updateCallbacks = updateCallbacks;
			job->jobExclusions.clear();
		}
		else {
			Ref<Job> newJob = Ref<Job>::make(jobName, updateCallbacks);
			newJob->system = this;
			jobs.push_back(newJob);
		}
	}

	void JobSystem::addJobExclusion(const std::string& jobName, const std::vector<std::string>& exclusions) {
		if (auto job = getJob(jobName)) {
			job->jobExclusions.insert(job->jobExclusions.end(), exclusions.begin(), exclusions.end());
		}
	}

	void JobSystem::initNewJobs() {
		if (enableMultiThreading) {
			if (!startBarrier || barrierSize != jobs.size() + 1) {
				oldStartBarrier = startBarrier;
				startBarrier = Ref<std::barrier<>>::make(jobs.size() + 1);
				if (!oldStartBarrier) {
					endBarrier = Ref<std::barrier<>>::make(jobs.size() + 1);
				}
				barrierSize = jobs.size() + 1;
			}
			running = true;
			for (auto job : jobs) {
				if (job && !job->initFlag) {
					job->threadId = env->threads->addThread([&, job]() {
						std::string profileName = job->name + " Thread";
						TRI_PROFILE_THREAD(profileName.c_str());
						while (running) {
							startBarrier->arrive_and_wait();
							if (running) {
								job->update();
							}
							endBarrier->arrive_and_wait();
						}
					});
					job->initFlag = true;
				}

			}
		}
		else {
			for (auto job : jobs) {
				if (job && !job->initFlag) {
					job->initFlag = true;
				}
			}
		}
	}

	JobSystem::Job *JobSystem::getJob(const std::string& jobName) {
		for (auto job : jobs) {
			if (job) {
				if (job->name == jobName) {
					return job.get();
				}
			}
		}
		return nullptr;
	}

	void JobSystem::Job::update() {
		for (auto& exclusion : jobExclusions) {
			if (auto* job = system->getJob(exclusion)) {
				job->mutex.lock();
				job->mutex.unlock();
			}
		}
		std::unique_lock<std::mutex> lock(mutex);

		for (auto& job : system->jobs) {
			if (job && job.get() != this) {
				for (auto& exclusion : job->jobExclusions) {
					if (exclusion == name) {
						job->mutex.lock();
						job->mutex.unlock();
					}
				}
			}
		}


		std::string profileName = name + " Job";
		TRI_PROFILE_NAME(profileName.c_str(), profileName.size());

		if (isDefaultJob) {
			for (auto& observer : env->signals->update.getObservers()) {
				bool handlesByOtherFiber = false;
				for (auto& update : updateCallbacks) {
					if (observer.name == update) {
						handlesByOtherFiber = true;
						break;
					}
				}
				if (!handlesByOtherFiber) {
					if (observer.active) {
						if (observer.callback) {
							TRI_PROFILE_NAME(observer.name.c_str(), observer.name.size());
							observer.callback();
						}
					}
				}
			}
		}
		else {
			for (auto& update : updateCallbacks) {
				for (auto& observer : env->signals->update.getObservers()) {
					if (observer.name == update) {
						if (observer.active) {
							if (observer.callback) {
								TRI_PROFILE_NAME(observer.name.c_str(), observer.name.size());
								observer.callback();
							}
						}
					}
				}
			}
		}
	}

}
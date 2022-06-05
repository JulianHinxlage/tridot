//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "MainLoop.h"
#include "Environment.h"
#include "Console.h"
#include "EventManager.h"
#include "CrashHandler.h"
#include "ModuleManager.h"
#include "Profiler.h"
#include "JobManager.h"
#include "CrashHandler.h"
#include "SystemManager.h"

#include <iostream>
#include <csetjmp>

namespace tri {

	void MainLoop::init() {
		Environment::init();
		env->console->addCVar<bool>("running", true);

		Console::LogTarget target;
		target.stream = &std::cout;
		target.includeDate = false;
		target.level = LogLevel::DEBUG;
		env->console->addLogTarget(target);

		Console::LogTarget target2;
		target2.file = "log.txt";
		target2.level = LogLevel::TRACE;
		env->console->addLogTarget(target2);
	}
	
	void MainLoop::startup() {
		env->moduleManager->performePending();
		env->eventManager->preStartup.invoke();
		env->jobManager->startupJobs();
		env->jobManager->startupPendingSystems(true);
		env->eventManager->postStartup.invoke();
	}

	void MainLoop::run() {
		while (env->console->getCVarValue("running", false)) {
			TRI_PROFILE_FRAME;
			env->systemManager->getSystem<Profiler>()->nextFrame();

			//performe startup of new systems
			if (env->systemManager->hasPendingStartups()) {
				env->jobManager->startupPendingSystems(false);
			}

			//tick jobs which will tick systems
			env->eventManager->preTick.invoke();
			env->jobManager->tickJobs();
			env->eventManager->postTick.invoke();

			//performe shutdown on systems to be removed
			if (env->systemManager->hasPendingShutdowns()) {
				env->jobManager->shutdownPendingSystems(false);
			}

			//load and unload pending modules
			env->moduleManager->performePending();

		}
	}
	
	void MainLoop::shutdown() {
		env->eventManager->preShutdown.invoke();

		//mark all systems for shutdown
		for (auto* desc : Reflection::getDescriptors()) {
			if (desc && (desc->flags & ClassDescriptor::SYSTEM)) {
				env->systemManager->getSystemHandle(desc->classId)->pendingShutdown = true;
			}
		}

		env->jobManager->shutdownPendingSystems(true);
		env->jobManager->shutdownJobs();
		env->eventManager->postShutdown.invoke();
		Environment::shutdown();

#if TRACY_ENABLE
		//close tracy connection
		if (TracyIsConnected) {
			tracy::GetProfiler().RequestShutdown();
			while (!tracy::GetProfiler().HasShutdownFinished()) {}
		}
#endif
	}

}

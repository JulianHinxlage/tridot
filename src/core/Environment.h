//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "System.h"
#include "Reflection.h"
#include "SystemManager.h"
#include "pch.h"

namespace tri {

	class Environment {
	public:
		//core
		class Reflection* reflection;
		class SystemManager* systemManager;
		class Console* console;
		class ModuleManager* moduleManager;
		class EventManager* eventManager;
		class JobManager* jobManager;
		class ThreadManager* threadManager;
		class FileWatcher *fileWatcher;
		class Profiler* profiler;
		class Config* config;
		
		//render
		class Window* window;

		Environment();

		static void init();
		static void shutdown();
	};

	Environment* getEnvironment();

}

static tri::Environment* env = tri::getEnvironment();

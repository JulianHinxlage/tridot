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
		
		//window
		class Window* window;
		class Viewport* viewport;

		//engine
		class Input* input;
		class Time* time;
		class Random* random;
		class AssetManager* assetManager;
		class Serializer* serializer;
		class RuntimeMode* runtimeMode;

		//entity
		class World* world;
		std::string worldFile;

		//editor
		class Editor* editor;

		Environment();

		static void init();
		static void shutdown();
	};

	Environment* getEnvironment();

}

static tri::Environment* env = tri::getEnvironment();

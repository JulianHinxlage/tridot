//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Environment.h"
#include "SystemManager.h"

namespace tri {

	Environment::Environment() {
		reflection = nullptr;
		systemManager = nullptr;
		console = nullptr;
		moduleManager = nullptr;
		eventManager = nullptr;
		jobManager = nullptr;
		threadManager = nullptr;
		fileWatcher = nullptr;
		profiler = nullptr;
		config = nullptr;

		window = nullptr;
		viewport = nullptr;
		input = nullptr;
		time = nullptr;
		random = nullptr;
		serializer = nullptr;
		assetManager = nullptr;
		runtimeMode = nullptr;
		world = nullptr;
		editor = nullptr;
	}

	void Environment::init() {
		if (!env->reflection) {
			env->reflection = new Reflection();
		}
		if (!env->systemManager) {
			env->systemManager = new SystemManager();
		}
		env->systemManager->addNewSystems();
	}

	void Environment::shutdown() {
		if (env->systemManager) {
			env->systemManager->removeAllSystems();
		}
		delete env->systemManager;
		env->systemManager = nullptr;
		delete env->reflection;
		env->reflection = nullptr;
	}

	Environment* getEnvironment() {
		static Environment environment;
		return &environment;
	}

}

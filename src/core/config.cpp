//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "config.h"
#include "Reflection.h"
#include "Console.h"
#include "Environment.h"
#include "ModuleManager.h"
#include "EventManager.h"
#include "CrashHandler.h"
#include "ThreadManager.h"
#include "JobManager.h"

namespace tri {

	TRI_SYSTEM_INSTANCE(Config, env->config);

	namespace impl {
		void assertLog(const std::string& message) {
			env->console->fatalSource("Assert", "%s", message.c_str());
		}
	}

	void Config::init() {
		env->console->addCVar<int>("windowWidth", 800);
		env->console->addCVar<int>("windowHeight", 600);
		env->console->addCVar<std::string>("windowTitle", "Tridot Engine");
		env->console->addCVar<int>("monitor", -1);
		env->console->addCVar<bool>("noWindow", false);
		env->console->addCVar<bool>("vsync", true);

		env->console->addCVar<bool>("enableModuleHotReloading", &env->moduleManager->enableModuleHotReloading);
		env->console->addCVar<bool>("unloadModuleOnCrash", &env->systemManager->getSystem<CrashHandler>()->unloadModuleOnCrash);
		env->console->addCVar<bool>("enableCrashRecovery", &env->systemManager->getSystem<CrashHandler>()->enableCrashRecovery);
		env->console->addCVar<bool>("enableJobMultithreading", &env->jobManager->enableMultithreading);
		env->console->addCVar<int>("workerThreadCount", &env->threadManager->workerThreadCount);

		env->console->addCommand("loadModule", [&](auto& args) {
			if (args.size() > 0) {
				env->moduleManager->loadModule(args[0], isPostStartup);
			}
		});
		env->console->addCommand("unloadModule", [&](auto& args) {
			if (args.size() > 0) {
				env->moduleManager->unloadModule(args[0], isPostStartup);
			}
		});
		env->console->addCommand("addModuleDirectory", [&](auto& args) {
			if (args.size() > 0) {
				env->moduleManager->addModuleDirectory(args[0]);
			}
		});

		env->console->addCommand("loadConfig", [&](auto& args) {
			if (args.size() > 0) {
				loadConfigFile(args[0]);
			}
		});

		env->console->addCommand("exit", [&](auto& args) {
			env->console->setCVarValue<bool>("running", false);
		});
		env->console->addCommand("quit", [&](auto& args) {
			env->console->setCVarValue<bool>("running", false);
		});

		env->console->addCommand("delayed", [&](auto& args) {
			postStartupCommands.push_back(StrUtil::join(args, " "));
		});
		env->eventManager->postStartup.addListener([&]() {
			for (auto& command : postStartupCommands) {
				env->console->executeCommand(command);
			}
			isPostStartup = true;
		});
	}

	void Config::loadConfigFile(const std::string& file) {
		if (!std::filesystem::exists(file)) {
			env->console->info("config file \"%s\" not found", file.c_str());
			return;
		}
		env->console->info("loading config file \"%s\"", file.c_str());

		std::string config = StrUtil::readFile(file);

		std::filesystem::path currentPath = std::filesystem::current_path();
		std::filesystem::current_path(std::filesystem::path(file).parent_path());
		
		bool isOriginalPath = originalPath.empty();
		if (isOriginalPath) {
			originalPath = currentPath.string();
		}

		for (auto line : StrUtil::split(config, "\n", false)) {
			if (line.size() > 0 && line[0] != '#') {
				line = StrUtil::replace(line, "$", originalPath);
				env->console->executeCommand(line);
			}
		}

		if (isOriginalPath) {
			originalPath = "";
		}

		std::filesystem::current_path(currentPath);
	}

	void Config::loadConfigFileFirstFound(const std::vector<std::string>& fileList) {
		for (auto& file : fileList) {
			if (std::filesystem::exists(file)) {
				loadConfigFile(file);
				break;
			}
		}
	}


}



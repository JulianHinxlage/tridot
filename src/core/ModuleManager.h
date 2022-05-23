//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "System.h"
#include "Reflection.h"

namespace tri {

	class Module {
	public:
		void* handle;
		std::string name;
		std::string file;
		std::string path;
		std::string runtimeName;
		std::string runtimeFile;
		std::string runtimePath;
	};

	class ModuleManager : public System {
	public:
		bool enableModuleHotReloading = false;

		Module* loadModule(const std::string& name, bool pending = true);
		Module* getModule(const std::string& name);
		void unloadModule(const std::string& name, bool pending = true);
		void unloadModule(Module *module, bool pending = true);
		void performePending();

		const std::vector<std::shared_ptr<Module>> &getModules();
		static std::string getModuleNameByAddress(void* address);
	private:
		std::vector<std::shared_ptr<Module>> modules;
		std::vector<std::string> pendingLoads;
		std::vector<std::string> pendingUnloads;
	};

}

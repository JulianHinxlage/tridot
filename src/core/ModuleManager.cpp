//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "ModuleManager.h"
#include "config.h"
#include "Environment.h"
#include "Console.h"
#include "EventManager.h"
#include "Profiler.h"
#include "FileWatcher.h"

#if !TRI_WINDOWS
#include <dlfcn.h>
#else
#include <windows.h>
#endif

namespace tri {

	TRI_SYSTEM_INSTANCE(ModuleManager, env->moduleManager);

	bool checkName(Module* module, const std::string& file) {
		if (module->name == file) {
			return true;
		}
		if (module->file == file) {
			return true;
		}
		if (module->path == file) {
			return true;
		}
		if (module->runtimeName == file) {
			return true;
		}
		if (module->runtimePath == file) {
			return true;
		}
		return false;
	}

	void ModuleManager::init() {
		currentlyLoading = nullptr;
		env->eventManager->onClassRegister.addListener([this](int classId) {
			auto name = getModuleNameByAddress(Reflection::getDescriptor(classId)->registrationSourceAddress);
			if (!getModule(name)) {
				bool enabled = enableModuleHotReloading;
				Module *currently = currentlyLoading;
				enableModuleHotReloading = false;
				Module *module = loadModule(name, false);
				if (currently) {
					currently->autoLoaded.push_back(module);
				}
				currentlyLoading = currently;
				enableModuleHotReloading = enabled;
			}
		});
	}

	Module* ModuleManager::loadModule(const std::string& name, bool pending) {
		if (pending) {
			pendingLoads.push_back(name);
			return nullptr;
		}
		
		std::string filePath = name;
		if (std::filesystem::path(name).extension() == "") {
#if TRI_WINDOWS
			filePath = name + ".dll";
#else
			filePath = name + ".so";
#endif
		}

		TRI_PROFILE_FUNC();
		TRI_PROFILE_INFO(filePath.c_str(), filePath.size());
		if (!std::filesystem::exists(filePath)) {
			env->console->warning("module \"%s\" not found", name.c_str());
			return nullptr;
		}

		if (auto *module = getModule(name)) {
			env->console->info("module \"%s\" already loaded", name.c_str());
			return module;
		}

		std::string runtimePath = filePath;
		if (enableModuleHotReloading) {
			std::filesystem::path path(filePath);
			runtimePath = (path.parent_path() / "runtime_dlls" / path.filename()).string();
			if (!std::filesystem::exists(std::filesystem::path(runtimePath).parent_path())) {
				std::filesystem::create_directories(std::filesystem::path(runtimePath).parent_path());
			}
			int postfix = 0;
			for (int postfix = 0; postfix < 10; postfix++) {
				try {
					std::filesystem::copy(filePath, runtimePath, std::filesystem::copy_options::overwrite_existing);
					break;
				}
				catch (...) {}
				runtimePath = (path.parent_path() / "runtime_dlls" / (path.filename().stem().string() + "_" + std::to_string(postfix + 1) + path.filename().extension().string())).string();
			}
		}

		if (!std::filesystem::exists(runtimePath)) {
			runtimePath = filePath;
			env->console->info("can't create runtime file for module \"%s\"", name.c_str());
		}

		auto module = std::make_shared<Module>();
		module->path = filePath;
		module->file = std::filesystem::path(filePath).filename().string();
		module->name = std::filesystem::path(filePath).filename().stem().string();
		module->runtimePath = runtimePath;
		module->runtimeFile = std::filesystem::path(runtimePath).filename().string();
		module->runtimeName = std::filesystem::path(runtimePath).filename().stem().string();
		module->handle = nullptr;
		modules.push_back(module);
		currentlyLoading = module.get();


		{
#if TRI_WINDOWS
		TRI_PROFILE("loadDLL");
		module->handle = (void*)LoadLibrary(runtimePath.c_str());
		if (!module->handle) {
			env->console->info("faild to load module \"%s\" (code %i)", name.c_str(), GetLastError());
		}
#else
		TRI_PROFILE("loadSharedLibrary");
		module->handle = dlopen(runtimePath.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
		}




		SystemManager::addNewSystems();

		env->console->info("module \"%s\" loaded", module->name.c_str());
		env->eventManager->onModuleLoad.invoke(module->name);

		if (enableModuleHotReloading) {
			env->fileWatcher->addFile(module->path, [](const std::string& path) {
				env->console->info("reloading module \"%s\"", std::filesystem::path(path).filename().stem().string().c_str());
				env->moduleManager->unloadModule(path, true);
				env->moduleManager->loadModule(path, true);
			});
		}

		currentlyLoading = nullptr;
		return module.get();
	}

	Module* ModuleManager::getModule(const std::string& name) {
		for (auto& module : modules) {
			if (module && checkName(module.get(), name)) {
				return module.get();
			}
		}
		return nullptr;
	}

	const std::vector<std::shared_ptr<Module>>& ModuleManager::getModules() {
		return modules;
	}

	void ModuleManager::unloadModule(const std::string& name, bool pending) {
		unloadModule(getModule(name), pending);
	}

	void ModuleManager::unloadModule(Module* module, bool pending) {
		for (int i = 0; i < modules.size(); i++) {
			auto* m = modules[i].get();
			if (m && m == module) {
				if (pending) {
					pendingUnloads.push_back(m->file);

					//mark systems for shutdown
					for (auto* desc : Reflection::getDescriptors()) {
						if (desc && (desc->flags & ClassDescriptor::SYSTEM)) {
							std::string file = getModuleNameByAddress(desc->registrationSourceAddress);
							if (checkName(m, file)) {
								env->systemManager->getSystemHandle(desc->classId)->pendingShutdown = true;
							}
						}
					}
					return;
				}

				TRI_PROFILE_FUNC();
				TRI_PROFILE_INFO(m->file.c_str(), m->file.size());

				env->eventManager->onModuleUnload.invoke(module->name);

				//remove systems and unregister types assosiated with the unloaded module
				for (auto* desc : Reflection::getDescriptors()) {
					if (desc) {
						std::string file = getModuleNameByAddress(desc->registrationSourceAddress);
						if (checkName(m, file)) {
							if (desc->flags & ClassDescriptor::SYSTEM) {
								env->systemManager->removeSystem(desc->classId, false, true);
							}
							Reflection::unregisterClass(desc->classId);
						}
					}
				}

				{
#ifdef TRI_WINDOWS
					TRI_PROFILE("unloadDLL");
					FreeLibrary((HINSTANCE)m->handle);
#else      
					TRI_PROFILE("unloadSharedLibrary");
					dlclose(m->handle);
#endif
				}

				env->console->info("module \"%s\" unloaded", module->name.c_str());

				modules.erase(modules.begin() + i);
				break;
			}
		}
	}

	void ModuleManager::performePending() {
		for (auto& file : pendingUnloads) {
			unloadModule(file, false);
		}
		pendingUnloads.clear();

		for (auto& file : pendingLoads) {
			loadModule(file, false);
		}
		pendingLoads.clear();
	}

	std::string ModuleManager::getModuleNameByAddress(void* address) {
#if TRI_WINDOWS
		char path[256];
		HMODULE hm = NULL;

		if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
			GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(LPCSTR)address, &hm) == 0) {
			return "";
		}
		if (GetModuleFileName(hm, path, sizeof(path)) == 0) {
			return "";
		}

		return std::filesystem::path(path).filename().string();
#else
		return "";
#endif
	}



}

//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "SystemManager.h"
#include "Reflection.h"
#include "Profiler.h"

namespace tri {

	static std::vector<SystemManager::SystemHandle>& getSystemsImpl() {
		static std::vector<SystemManager::SystemHandle> systems;
		return systems;
	}

	System* SystemManager::getSystem(int classId) {
		if (getSystemsImpl().size() > classId) {
			return getSystemsImpl()[classId].system;
		}
		return nullptr;
	}

	System* SystemManager::addSystem(int classId) {
		TRI_PROFILE_FUNC();
		if (getSystemsImpl().size() <= classId) {
			getSystemsImpl().resize(classId + 1);
		}
		auto& handle = getSystemsImpl()[classId];
		if (handle.system == nullptr) {
			handle.name = Reflection::getDescriptor(classId)->name;
			TRI_PROFILE_NAME(handle.name.c_str(), handle.name.size());
			handle.system = (System*)Reflection::getDescriptor(classId)->alloc();
			handle.system->init();
			handle.wasInit = true;
			handle.wasStartup = false;
			handle.wasShutdown = false;
			handle.pendingShutdown = false;

			if (handle.instancePointer != nullptr) {
				*handle.instancePointer = handle.system;
			}
		}
		return handle.system;
	}

	void SystemManager::removeSystem(int classId, bool pending, bool canAutoAddAgain) {
		TRI_PROFILE_FUNC();
		if (getSystemsImpl().size() <= classId) {
			return;
		}
		auto& handle = getSystemsImpl()[classId];
		if (handle.system) {
			if (pending) {
				handle.pendingShutdown = true;
				if (canAutoAddAgain) {
					handle.wasAutoAdd = false;
				}
				return;
			}

			TRI_PROFILE_NAME(handle.name.c_str(), handle.name.size());
			if (!handle.wasShutdown) {
				handle.system->shutdown();
			}
			Reflection::getDescriptor(classId)->free(handle.system);
			handle.system = nullptr;

			if (canAutoAddAgain) {
				handle.wasAutoAdd = false;
			}

			if (handle.instancePointer != nullptr) {
				*handle.instancePointer = handle.system;
			}
		}
	}

	void SystemManager::setSystemPointer(int classId, void** ptr) {
		if (getSystemsImpl().size() <= classId) {
			getSystemsImpl().resize(classId + 1);
		}
		auto& handle = getSystemsImpl()[classId];
		handle.instancePointer = ptr;
		if (handle.system) {
			if (handle.instancePointer) {
				*handle.instancePointer = handle.system;
			}
		}
	}

	SystemManager::SystemHandle* SystemManager::getSystemHandle(int classId) {
		if (getSystemsImpl().size() <= classId) {
			return nullptr;
		}
		return &getSystemsImpl()[classId];
	}

	bool SystemManager::hasPendingStartups() {
		auto& systems = getSystemsImpl();
		for (auto& system : systems) {
			if (system.system) {
				if (!system.wasStartup) {
					return true;
				}
			}
		}
		return false;
	}

	bool SystemManager::hasPendingShutdowns() {
		auto& systems = getSystemsImpl();
		for (auto& system : systems) {
			if (system.system) {
				if (system.pendingShutdown && !system.wasShutdown) {
					return true;
				}
			}
		}
		return false;
	}

	void SystemManager::addNewSystems() {
		auto& systems = getSystemsImpl();
		for (auto* desc : Reflection::getDescriptors()) {
			if (desc && (desc->flags & ClassDescriptor::SYSTEM)){
				if (systems.size() <= desc->classId || !systems[desc->classId].wasAutoAdd) {
					addSystem(desc->classId);
					systems[desc->classId].wasAutoAdd = true;
				}
			}
		}
	}

	void SystemManager::removeAllSystems() {
		auto& systems = getSystemsImpl();
		for (int i = systems.size() - 1; i >= 0; i--){
			removeSystem(i, false);
		}
		getSystemsImpl().clear();
	}

}
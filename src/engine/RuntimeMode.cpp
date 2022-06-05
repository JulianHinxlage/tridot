//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "RuntimeMode.h"

namespace tri {

	TRI_SYSTEM_INSTANCE(RuntimeMode, env->runtimeMode);

	RuntimeMode::Mode RuntimeMode::getMode() {
		return mode;
	}

	void RuntimeMode::setMode(Mode mode) {
		Mode previousMode = this->mode;
		if (mode == previousMode) {
			return;
		}
		this->mode = mode;
		auto entry = modeContexts.find(mode);
		if (entry != modeContexts.end()) {
			auto &context = entry->second;

			if (context.activeSystems.size() > 0) {
				setAllActive(false);
				for (auto& active : context.activeSystems) {
					auto* desc = Reflection::getDescriptor(active);
					if (desc) {
						auto* system = env->systemManager->getSystemHandle(desc->classId);
						if (system) {
							system->active = true;
						}
					}
				}
			}
			else {
				setAllActive(true);
				for (auto& inactive : context.inactiveSystems) {
					auto* desc = Reflection::getDescriptor(inactive);
					if (desc) {
						auto* system = env->systemManager->getSystemHandle(desc->classId);
						if (system) {
							system->active = false;
						}
					}
				}
			}

		}
		else {
			setAllActive(true);
		}
		env->eventManager->onRuntimeModeChange.invoke(previousMode, mode);
	}

	void RuntimeMode::setActiveSystem(Mode mode, const std::string& systemName, bool active) {
		auto &context = modeContexts[mode];
		if (active) {
			context.activeSystems.push_back(systemName);
		}
		else {
			context.inactiveSystems.push_back(systemName);
		}
	}

	void RuntimeMode::setActiveSystems(Mode mode, const std::vector<std::string>& systemNames, bool active) {
		for (auto& systemName : systemNames) {
			setActiveSystem(mode, systemName, active);
		}
	}

	void RuntimeMode::setAllActive(bool active) {
		for (auto* desc : Reflection::getDescriptors()) {
			if (desc && desc->flags & ClassDescriptor::SYSTEM) {
				auto* sys = env->systemManager->getSystemHandle(desc->classId);
				if (sys) {
					sys->active = active;
				}
			}
		}
	}

}
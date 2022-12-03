//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"

namespace tri {

	class RuntimeMode : public System {
	public:
		enum Mode {
			PLAY,
			PAUSED,
			EDIT,
			LOADING,
		};

		void init() override;
		void startup() override;
		void shutdown() override;
		Mode getMode();
		void setMode(Mode mode);

		void setActiveSystem(Mode mode, const std::string& systemName, bool active);
		void setActiveSystems(Mode mode, const std::vector<std::string>& systemNames, bool active);
		void setActiveSystem(const std::vector<Mode>& modes, const std::string& systemName, bool active);
		void setActiveSystems(const std::vector<Mode> &modes, const std::vector<std::string>& systemNames, bool active);

		template<typename T>
		void setActiveSystem(Mode mode, bool active) {
			setActiveSystem(mode, Reflection::getDescriptor<T>()->name, active);
		}
		template<typename T>
		void setActiveSystem(const std::vector<Mode>& modes, bool active) {
			setActiveSystem(modes, Reflection::getDescriptor<T>()->name, active);
		}
		template<typename... T>
		void setActiveSystems(Mode mode, bool active) {
			(setActiveSystem<T>(mode, active), ...);
		}
		template<typename... T>
		void setActiveSystems(const std::vector<Mode>& modes, bool active) {
			(setActiveSystem<T>(modes, active), ...);
		}

	private:
		Mode mode = LOADING;
		int listener;
		
		class ModeContext {
		public:
			std::vector<std::string> activeSystems;
			std::vector<std::string> inactiveSystems;
		};

		std::unordered_map<Mode, ModeContext> modeContexts;

		void setAllActive(bool active);
		void updateActive();
	};

}

#define TRI_COMPONENT_TICK(T) TRI_TICK(){if(env->runtimeMode->getMode() != tri::RuntimeMode::PLAY){return;} env->world->each<T>([](EntityId id, T& t) { t.tick(id); }); }

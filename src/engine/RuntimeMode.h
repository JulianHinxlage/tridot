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

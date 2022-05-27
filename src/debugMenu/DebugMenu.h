//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "core/System.h"
#include "core/Reflection.h"

namespace tri {

	class DebugMenu : public System {
	public:
		bool active;

		void init() override;
		void startup() override;
		void tick() override;

		template<typename T>
		void addWindow(const std::string& displayName) {
			addWindow(Reflection::getClassId<T>(), displayName);
		}

		void addWindow(int classId, const std::string& displayName);

	private:
		class Window {
		public:
			int classId;
			std::string displayName;
			bool active;
		};

		std::vector<Window> windows;

		void setupFlagsHandler();
	};

}

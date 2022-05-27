//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "core/System.h"
#include "core/Reflection.h"
#include "entity/World.h"

namespace tri {

	class Editor : public System {
	public:
		void init() override;
		void startup() override;
		void tick() override;

		template<typename T>
		void addWindow(const std::string& displayName, const std::string& menu = "View", const std::string& category = "") {
			addWindow(Reflection::getClassId<T>(), displayName, menu, category);
		}
		void addWindow(int classId, const std::string& displayName, const std::string& menu = "View", const std::string& category = "");

		EntityId selectedEntity = -1;
	private:
		class Window {
		public:
			int classId;
			std::string displayName;
			std::string menu;
			std::string category;
		};

		std::vector<Window> windows;
		std::vector<std::string> menus;

		void setupFlagsHandler();
	};

}

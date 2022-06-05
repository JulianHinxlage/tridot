//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"

namespace tri {

	class UIWindow : public System {
	public:
		bool active;
	};

	class UIManager : public System {
	public:
		void init() override;
		void startup() override;
		void tick() override;
		void shutdown() override;
		void updateActiveFlags();

		template<typename T>
		void addWindow(const std::string& displayName, const std::string& menu = "View", const std::string& category = "") {
			addWindow(Reflection::getClassId<T>(), displayName, menu, category);
		}
		void addWindow(int classId, const std::string& displayName, const std::string& menu = "View", const std::string& category = "");

	private:
		bool active = true;

		class Window {
		public:
			int classId;
			std::string displayName;
			std::string menu;
			std::string category;
			UIWindow* window;
			bool active;
		};

		std::vector<Window> windows;
		std::vector<std::string> menus;
		std::vector<std::pair<std::string, bool>> unusedActiveFlags;

		void setupFlagHandler();
	};

}
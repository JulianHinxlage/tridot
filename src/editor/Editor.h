//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "core/core.h"

namespace tri {

	class EditorWindow {
	public:
		std::string name;
		bool isOpen = false;
		bool isDebugWindow = false;
		bool isWindow = true;

		virtual void startup() {}
		virtual void update() {}
		virtual void shutdown() {}
	};

	class Editor : public System {
	public:
		bool runtimeMode;

		void startup() override;
		void update() override;
		void shutdown() override;
		void addWindow(EditorWindow* window);

		void updateMenueBar();

	private:
		std::vector<EditorWindow*> windows;
		bool updated;
	};

	extern Editor* editor;

}



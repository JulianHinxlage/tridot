//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/System.h"
#include <glm/glm.hpp>

namespace tri {

	class Window : public System {
	public:
		void init() override;
		void startup() override;
		void shutdown() override;
		void tick() override;

		void setVSync(int interval);
		int getVSync();
		void setBackgroundColor(const glm::vec4 &color);

		glm::vec2 getPosition();

		void close();
		bool isOpen();
		bool inFrame();
		void* getContext();
		void* getNativeContext();
	private:
		void *window;
		bool inFrameFlag = false;
		int vsyncInterval = 1;
		glm::vec4 backgroundColor = {1, 1, 1, 1};

		void updateBegin();
		void updateEnd();
	};

}


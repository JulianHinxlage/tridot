//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/System.h"

namespace tri {

	class Window : public System {
	public:
		void init() override;
		void startup() override;
		void shutdown() override;
		void tick() override;

		void setVSync(int interval);
		int getVSync();

		void close();
		bool isOpen();
		bool inFrame();
		void* getContext();
		void* getNativeContext();
	private:
		void *window;
		bool inFrameFlag = false;
		int vsyncInterval = 1;

		void updateBegin();
		void updateEnd();
	};

}


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
		void updateBegin();
		void updateEnd();
		void shutdown() override;
		void tick() override;

		void close();
		bool isOpen();
		bool inFrame();
	private:
		void *window;
		bool inFrameFlag = false;
	};

}


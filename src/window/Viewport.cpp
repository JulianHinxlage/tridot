//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Viewport.h"

namespace tri {

	TRI_SYSTEM_INSTANCE(Viewport, env->viewport);

	void Viewport::init() {
		env->jobManager->addJob("Render")->addSystem<Viewport>();
	}

	void Viewport::shutdown() {
		frameBuffer = nullptr;
	}

}
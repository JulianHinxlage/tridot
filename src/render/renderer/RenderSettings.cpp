//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "RenderSettings.h"

namespace tri {

	TRI_SYSTEM_INSTANCE(RenderSettings, env->renderSettings);

    void RenderSettings::init() {
        env->console->addCVar("enableShadows", &enableShadows);
        env->console->addCVar("enableBloom", &enableBloom);
        env->console->addCVar("enableSSAO", &enableSSAO);
    }

	RenderSettings::Statistics::Statistics() {
        triangleCount = 0;
        instanceCount = 0;
        drawCallCount = 0;
        shaderCount = 0;
        meshCount = 0;
        lightCount = 0;
        cameraCount = 0;
        materialCount = 0;
	}

}

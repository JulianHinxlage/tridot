//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "RenderSettings.h"

namespace tri {

	TRI_SYSTEM_INSTANCE(RenderSettings, env->renderSettings);

    void RenderSettings::init() {
        env->console->addCVar("enableTransparency", &enableTransparency);
        env->console->addCVar("enablePointLights", &enablePointLights);
        env->console->addCVar("enableSpotLights", &enableSpotLights);
        env->console->addCVar("enableFrustumCulling", &enableFrustumCulling);
        env->console->addCVar("enableShadows", &enableShadows);

        env->console->addCVar("enableBloom", &enableBloom);
        env->console->addCVar("bloomThreshold", &bloomThreshold);
        env->console->addCVar("bloomIntesity", &bloomIntesity);
        env->console->addCVar("bloomSpread", &bloomSpread);

        env->console->addCVar("enableSSAO", &enableSSAO);
        env->console->addCVar("ssaoKernalSize", &ssaoKernalSize);
        env->console->addCVar("ssaoSampleRadius", &ssaoSampleRadius);
        env->console->addCVar("ssaoBias", &ssaoBias);
        env->console->addCVar("ssaoOcclusionStrength", &ssaoOcclusionStrength);

        env->console->addCVar("hueShift", &hueShift);
        env->console->addCVar("saturation", &saturation);
        env->console->addCVar("temperature", &temperature);
        env->console->addCVar("contrast", &contrast);
        env->console->addCVar("brightness", &brightness);
        env->console->addCVar("gamma", &gamma);
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

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"
#include <glm/glm.hpp>

namespace tri {

    class RenderSettings : public System {
    public:
        bool enableTransparency = true;
        bool enablePointLights = true;
        bool enableSpotLights = true;
        bool enableFrustumCulling = true;

        bool enableShadows = true;
        int shadowMapResolution = 2048;

        bool enableBloom = true;
        float bloomThreshold = 1;
        float bloomIntesity = 1;
        int bloomSpread = 20;

        bool enableSSAO = true;
        int ssaoKernalSize = 32;
        float ssaoSampleRadius = 1.0;
        float ssaoBias = 0.025;
        float ssaoOcclusionStrength = 1.0;

        float hueShift = 0.0f;
        float saturation = 1.0f;
        float temperature = 1.0f;
        float contrast = 1.0f;
        float brightness = 1.0f;
        float gamma = 1.0f;
        glm::vec3 gain = { 0, 0, 0 };

        virtual void init() override;

        class Statistics {
        public:
            int triangleCount;
            int instanceCount;
            int drawCallCount;
            int shaderCount;
            int meshCount;
            int lightCount;
            int cameraCount;
            int materialCount;

            Statistics();
        };

        Statistics statistics;
    };

}

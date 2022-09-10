//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"

namespace tri {

    class RenderSettings : public System {
    public:
        bool enableTransparency = true;
        bool enablePointLights = true;
        bool enableSpotLights = true;
        bool enableBloom = true;
        bool enableShadows = false;
        bool enableSSAO = false;

        float bloomThreshold = 1;
        float bloomIntesity = 1;
        int bloomSpread = 20;

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

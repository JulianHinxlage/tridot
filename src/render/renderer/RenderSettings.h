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
        bool enableShadows = false;
        bool enableBloom = false;
        bool enableSSAO = false;

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

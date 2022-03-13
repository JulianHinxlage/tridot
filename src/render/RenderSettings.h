//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"

namespace tri {

	class RenderSettings {
	public:
        bool shadowsEnabled = true;
        int shadowMapResolution = 4096;
        bool drawListSortingEnabled = true;
        bool frustumCullingEnabled = true;
        bool deferredShadingEnabled = true;

        class Statistics {
        public:
            int drawCallCount = 0;
            int instanceCount = 0;
            int meshCount = 0;
            int materialCount = 0;
            int shaderCount = 0;
            int lightCount = 0;
            int cameraCount = 0;
        };
        Statistics stats;
	};

}
//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "Color.h"
#include "FrameBuffer.h"
#include "core/util/Ref.h"
#include <glm/glm.hpp>

namespace tri {

    enum LightType{
        AMBIENT_LIGHT = 0,
        DIRECTIONAL_LIGHT = 1,
        POINT_LIGHT = 2,
        SPOT_LIGHT = 3,
    };

    class Light {
    public:
        Color color;
        float intensity;
        LightType type;
        float radius;
        Ref<FrameBuffer> shadowMap;

        Light(LightType type = DIRECTIONAL_LIGHT, Color color = Color::white, float intensity = 1, float radius = 10);
    };

}


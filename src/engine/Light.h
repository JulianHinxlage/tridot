//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "Color.h"
#include "core/util/Ref.h"

namespace tri {

    class FrameBuffer;

    class AmbientLight {
    public:
        Color color = color::white;
        float intensity = 0.5;
    };

    class DirectionalLight {
    public:
        Color color = color::white;
        float intensity = 1.5;
        bool shadows = true;
        Ref<FrameBuffer> shadowMap;
    };

    class PointLight {
    public:
        Color color = color::white;
        float intensity = 20;
        float range = 5;
        float falloff = 2;
    };

    class SpotLight {
    public:
        Color color = color::white;
        float intensity = 50;
        float range = 20;
        float falloff = 1.5;
        float spotAngle = 30;
    };

}

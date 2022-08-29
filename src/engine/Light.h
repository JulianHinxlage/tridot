//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "Color.h"

namespace tri {

    class Light {
    public:
        enum Type {
            AMBIENT_LIGHT,
            DIRECTIONAL_LIGHT,
            POINT_LIGHT,
            SPOT_LIGHT,
        };
        Type type;
        Color color;
        float intensity;
        float range;
        float falloff;
        float spotAngle;

        Light();
    };

}

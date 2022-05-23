//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Light.h"
#include "core/core.h"

namespace tri {

    TRI_REGISTER_COMPONENT(Light);
    TRI_REGISTER_MEMBER(Light, type);
    TRI_REGISTER_MEMBER(Light, color);
    TRI_REGISTER_MEMBER(Light, intensity);
    TRI_REGISTER_MEMBER(Light, radius);
    TRI_REGISTER_MEMBER(Light, shadowMap);

    TRI_REGISTER_TYPE(LightType);
    TRI_REGISTER_CONSTANT(LightType, AMBIENT_LIGHT);
    TRI_REGISTER_CONSTANT(LightType, DIRECTIONAL_LIGHT);
    TRI_REGISTER_CONSTANT(LightType, POINT_LIGHT);
    TRI_REGISTER_CONSTANT(LightType, SPOT_LIGHT);

    Light::Light(LightType type, Color color, float intensity, float radius)
        : type(type), color(color), intensity(intensity), radius(radius) {
        shadowMap = nullptr;
    }

}

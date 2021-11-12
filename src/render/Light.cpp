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
    TRI_REGISTER_MEMBER(Light, shadowMap);

    TRI_REGISTER_TYPE(LightType);
    TRI_REGISTER_CONSTANT(LightType, AMBIENT_LIGHT);
    TRI_REGISTER_CONSTANT(LightType, DIRECTIONAL_LIGHT);
    TRI_REGISTER_CONSTANT(LightType, POINT_LIGHT);

    Light::Light(LightType type, Color color, float intensity)
        : type(type), color(color), intensity(intensity){
        shadowMap = nullptr;
    }

}

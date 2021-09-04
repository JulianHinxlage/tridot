//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Light.h"

namespace tridot {

    Light::Light(LightType type, const glm::vec3 &color, float intensity)
        : type(type), color(color), intensity(intensity){}

}

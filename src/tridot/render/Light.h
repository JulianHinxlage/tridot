//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <glm/glm.hpp>

namespace tridot {

    enum LightType{
        AMBIENT_LIGHT = 0,
        DIRECTIONAL_LIGHT = 1,
        POINT_LIGHT = 2,
    };

    class Light {
    public:
        glm::vec3 color;
        float intensity;
        LightType type;

        Light(LightType type = DIRECTIONAL_LIGHT, const glm::vec3 &color = {1, 1, 1}, float intensity = 1);
    };

}


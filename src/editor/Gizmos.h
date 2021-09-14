//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once
#include "engine/Transform.h"
#include "engine/Camera.h"
#include <glm/glm.hpp>

namespace tri {

    class Gizmos {
    public:
        enum Operation{
            TRANSLATE,
            SCALE,
            ROTATE,
        };

        enum Mode{
            LOCAL,
            WORLD,
        };

        enum Pivots{
            CENTER,
            OBJECTS,
        };

        Operation operation;
        Mode mode;
        Pivots pivots;

        void startup();
        bool update(const Transform &cameraTransform, const Camera &camera, const glm::vec2 &viewportPosition, const glm::vec2 &viewportSize);
    };

}


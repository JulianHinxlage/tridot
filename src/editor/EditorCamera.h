//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once
#include "engine/Transform.h"
#include "engine/Camera.h"

namespace tri {

    class EditorCamera {
    public:
        float speed = 2;
        glm::vec2 startMousePosition = {0, 0};

        enum Mode{
            GIMBALED,
            FREE,
            ORBIT,
        };
        Mode mode = GIMBALED;

        void update(Camera &camera, Transform &transform);
    };

}


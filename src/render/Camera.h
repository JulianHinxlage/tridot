//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "FrameBuffer.h"
#include <glm/glm.hpp>

namespace tri {

    class PerspectiveCamera {
    public:
        glm::vec3 forward;
        glm::vec3 up;
        glm::vec3 right;
        float fieldOfView;
        float aspectRatio;
        float near;
        float far;
        Ref<FrameBuffer> target;
        Ref<FrameBuffer> output;

        PerspectiveCamera();
        glm::mat4 getProjection();
    };

    class OrthographicCamera {
    public:
        glm::vec2 scale;
        glm::vec2 up;
        glm::vec2 right;
        float rotation;
        float aspectRatio;
        Ref<FrameBuffer> target;
        Ref<FrameBuffer> output;

        OrthographicCamera();
        glm::mat4 getProjection();
    };

}


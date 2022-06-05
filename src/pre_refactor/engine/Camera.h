//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/config.h"
#include "render/FrameBuffer.h"
#include <glm/glm.hpp>

namespace tri {

    class Camera {
    public:
        glm::vec3 forward;
        glm::vec3 up;
        glm::vec3 right;
        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 viewProjection;
        glm::mat4 transform;

        Ref<FrameBuffer> output;

        bool isPrimary;
        bool active;

        enum Type {
            PERSPECTIVE,
            ORTHOGRAPHIC,
        };
        Type type;
        float near;
        float far;
        float fieldOfView;
        float aspectRatio;

        Camera(Type type = PERSPECTIVE, bool isPrimary = false);
    };

}
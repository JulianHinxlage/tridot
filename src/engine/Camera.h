//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/config.h"
#include "core/util/Ref.h"
#include <glm/glm.hpp>

namespace tri {

    class FrameBuffer;

    class Camera {
    public:
        glm::vec3 forward;
        glm::vec3 up;
        glm::vec3 right;
        glm::vec3 eyePosition;
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

        glm::vec3 getScreenRay(glm::vec2 screenPosition);
    };

}

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_CAMERA_H
#define TRIDOT_CAMERA_H

#include "FrameBuffer.h"
#include <glm/glm.hpp>

namespace tridot {

    class PerspectiveCamera {
    public:
        glm::vec3 position;
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
        glm::mat4 getView();
        glm::mat4 getPerspective();
    };

    class OrthographicCamera {
    public:
        glm::vec2 position;
        glm::vec2 scale;
        glm::vec2 up;
        glm::vec2 right;
        float rotation;
        float aspectRatio;
        Ref<FrameBuffer> target;
        Ref<FrameBuffer> output;

        OrthographicCamera();
        glm::mat4 getProjection();
        glm::mat4 getView();
        glm::mat4 getPerspective();
    };

}

#endif //TRIDOT_CAMERA_H

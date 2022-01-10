//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "EditorElement.h"
#include "engine/Transform.h"
#include "engine/Camera.h"
#include <glm/glm.hpp>

namespace tri {

    class Gizmos : public EditorElement {
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

        bool snapping = false;
        bool snappingInvert = false;
        glm::vec3 translateSnapValues = {1, 1, 1};
        glm::vec3 scaleSnapValues = {0.25, 0.25, 0.25};
        glm::vec3 rotateSnapValues = {45, 45, 45};

        void startup() override;
        bool updateGizmo(const Transform &cameraTransform, const Camera &camera, const glm::vec2 &viewportPosition, const glm::vec2 &viewportSize);
        void updateSettings();

    private:
        std::vector<std::pair<EntityId,Transform>> preModifyValues;
        glm::mat4 preModifyMatrix;
        bool lastFrameUsing = false;
    };

}


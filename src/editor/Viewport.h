//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_VIEWPORT_H
#define TRIDOT_VIEWPORT_H

#include "EditorCamera.h"
#include "tridot/core/config.h"
#include <imgui/imgui.h>
#include <imguizmo/ImGuizmo.h>

namespace tridot {

    class Viewport {
    public:
        glm::vec2 viewportSize = {0, 0};
        glm::vec2 viewportPosition = {0, 0};
        EditorCamera editorCamera;
        ImGuizmo::OPERATION operation = ImGuizmo::OPERATION::TRANSLATE;
        ImGuizmo::MODE mode = ImGuizmo::MODE::LOCAL;
        bool snap = false;
        float snapValueTranslate = 0.5;
        float snapValueScale = 0.25;
        float snapValueRotate = 45.0;
        glm::vec2 mousePickPosition = {0, 0};
        bool controlDown = false;
        Ref<FrameBuffer> selectionOverlayTarget;
        bool selectionOverlay = true;

        void init();
        void clear();
        void update();
        void updateControlBar();
        void draw();
        void updateGizmos();
        void updateMousePicking();
        void drawSelectionOverlay();
    };

}

#endif //TRIDOT_VIEWPORT_H

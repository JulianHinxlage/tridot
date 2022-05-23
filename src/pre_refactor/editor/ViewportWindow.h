//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "EditorElement.h"
#include "EditorCamera.h"
#include "entity/Scene.h"
#include "entity/Prefab.h"
#include "render/Material.h"

namespace tri {

    enum EditorCameraMode{
        EDITOR_CAMERA,
        PRIMARY_CAMERA,
        FIXED_PRIMARY_CAMERA,
    };

    class ViewportWindow : public EditorElement {
    public:
        EntityId editorCameraId;
        EntityId drawCameraId;
        glm::vec2 viewportSize;
        glm::vec2 viewportPosition;
        bool isHovered;

        EditorCamera editorCamera;
        EditorCameraMode cameraMode;
        Prefab editorCameraBuffer;
        Ref<FrameBuffer> selectionOverlay;
        Ref<FrameBuffer> selectionOverlay2;
        Ref<Material> outlineMaterial;

        void startup();
        void setupEditorCamera();
        void update() override;
        void updateMousePicking(Ref<Texture> texture, glm::vec2 viewportSize, glm::vec2 pos);
        void updateSelectionOverlay(Transform& cameraTransform, Camera& camera, glm::vec2 viewportSize);
        void saveEditorCamera();
        void restoreEditorCamera();
    };

}


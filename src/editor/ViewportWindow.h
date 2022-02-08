//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "EditorElement.h"
#include "EditorCamera.h"
#include "entity/Scene.h"
#include "entity/Prefab.h"

namespace tri {

    enum EditorCameraMode{
        EDITOR_CAMERA,
        PRIMARY_CAMERA,
        FIXED_PRIMARY_CAMERA,
    };

    class ViewportWindow : public EditorElement {
    public:
        EntityId editorCameraId;
        EditorCamera editorCamera;
        EditorCameraMode cameraMode;
        Prefab editorCameraBuffer;
        Ref<Scene> sceneBuffer;
        Ref<FrameBuffer> selectionOverlay;

        void startup();
        void setupFrameBuffer(Camera &cam, bool idBuffer);
        void setupCamera();
        void update() override;
        void updateMousePicking(Ref<Texture> texture, glm::vec2 viewportSize, glm::vec2 pos);
        void updateSelectionOverlay(Transform& cameraTransform, Camera& camera, glm::vec2 viewportSize);
        void saveEditorCamera();
        void restoreEditorCamera();
    };

}


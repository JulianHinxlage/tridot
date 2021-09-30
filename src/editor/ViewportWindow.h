//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "EditorWindow.h"
#include "EditorCamera.h"
#include "entity/Scene.h"

namespace tri {

    enum EditorCameraMode{
        EDITOR_CAMERA,
        PRIMARY_CAMERA,
        FIXED_PRIMARY_CAMERA,
    };

    class ViewportWindow : public EditorWindow {
    public:
        EntityId editorCameraId;
        EditorCamera editorCamera;
        Ref<Scene> sceneBuffer;
        EditorCameraMode cameraMode;
        Transform editorCameraTransformBuffer;

        void startup();
        void setupFrameBuffer(Camera &cam, bool idBuffer);
        void setupCamera();
        void update() override;
        void updateMousePicking(Ref<Texture> texture, glm::vec2 viewportSize);
        void saveEditorCameraTransform();
        void restoreEditorCameraTransform();
    };

}

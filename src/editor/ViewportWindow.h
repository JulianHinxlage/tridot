//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "EditorWindow.h"
#include "EditorCamera.h"
#include "entity/Scene.h"

namespace tri {

    class ViewportWindow : public EditorWindow {
    public:
        EntityId editorCameraId;
        EditorCamera editorCamera;
        Ref<Scene> sceneBuffer;

        void startup();
        void setupFrameBuffer(Camera &cam, bool idBuffer);
        void setupCamera();
        void update() override;
        void updateMousePicking(Ref<Texture> texture, glm::vec2 viewportSize);
    };

}


//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "render/Renderer.h"
#include <imgui.h>

namespace tri{

    class StatisticsWindow : public EditorWindow {
    public:
        void startup() {
            name = "Statistics";
            isDebugWindow = true;
        }

        void update() override {
            ImGui::Text("entities:");
            ImGui::Text("%i entities in scene", env->scene->getEntityPool()->size());
            ImGui::Text("%i entities selected", (int)env->editor->selectionContext.getSelected().size());

            ImGui::Separator();
            ImGui::Text("rendering:");
            ImGui::Text("%i instances", env->renderer->instanceCount);
            ImGui::Text("%i draw calls", env->renderer->drawCallCount);
            ImGui::Text("%i shaders", env->renderer->shaderCount);
            ImGui::Text("%i materials", env->renderer->materialCount);
            ImGui::Text("%i meshes", env->renderer->meshCount);
            ImGui::Text("%i lights", env->renderer->lightCount);
            ImGui::Text("%i cameras", env->renderer->cameraCount);

            env->renderer->drawCallCount = 0;
            env->renderer->instanceCount = 0;
            env->renderer->meshCount = 0;
            env->renderer->materialCount = 0;
            env->renderer->shaderCount = 0;
            env->renderer->lightCount = 0;
            env->renderer->cameraCount = 0;
        }
    };

    TRI_STARTUP_CALLBACK("") {
        env->editor->addWindow(new StatisticsWindow);
    }

}
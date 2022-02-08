//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "render/Renderer.h"
#include "engine/Time.h"
#include <imgui.h>

namespace tri{

    class StatisticsWindow : public EditorElement {
    public:
        void startup() {
            name = "Statistics";
            type = DEBUG_WINDOW;
        }

        void update() override {
            TRI_PROFILE("Statistics");
            ImGui::Text("%.2f fps", env->time->framesPerSecond);
            ImGui::Separator();
            
            ImGui::Text("entities:");
            ImGui::Text("%i entities in scene", env->scene->getEntityPool()->size());
            ImGui::Text("%i entities selected", (int)env->editor->selectionContext.getSelected().size());

            ImGui::Separator();
            ImGui::Text("rendering:");
            ImGui::Text("%i instances", env->renderer->stats.instanceCount);
            ImGui::Text("%i draw calls", env->renderer->stats.drawCallCount);
            ImGui::Text("%i shaders", env->renderer->stats.shaderCount);
            ImGui::Text("%i materials", env->renderer->stats.materialCount);
            ImGui::Text("%i meshes", env->renderer->stats.meshCount);
            ImGui::Text("%i lights", env->renderer->stats.lightCount);
            ImGui::Text("%i cameras", env->renderer->stats.cameraCount);

            env->renderer->stats.drawCallCount = 0;
            env->renderer->stats.instanceCount = 0;
            env->renderer->stats.meshCount = 0;
            env->renderer->stats.materialCount = 0;
            env->renderer->stats.shaderCount = 0;
            env->renderer->stats.lightCount = 0;
            env->renderer->stats.cameraCount = 0;
        }
    };

    TRI_STARTUP_CALLBACK("") {
        env->editor->addElement<StatisticsWindow>();
    }

}
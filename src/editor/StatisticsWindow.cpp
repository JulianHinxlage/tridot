//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "render/Renderer.h"
#include "render/RenderSettings.h"
#include "engine/Time.h"
#include <imgui.h>

namespace tri{

    class StatisticsWindow : public EditorElement {
    public:
        float fps;

        void startup() {
            name = "Statistics";
            type = DEBUG_WINDOW;
            fps = 0;
        }

        void update() override {
            TRI_PROFILE("Statistics");

            if (env->time->frameTicks(0.5)) {
                fps = env->time->framesPerSecond;
            }

            ImGui::Text("%.2f fps", fps);
            ImGui::Separator();
            
            ImGui::Text("entities:");
            ImGui::Text("%i entities in scene", env->scene->getEntityPool()->size());
            ImGui::Text("%i entities selected", (int)env->editor->selectionContext.getSelected().size());

            ImGui::Separator();
            ImGui::Text("rendering:");
            ImGui::Text("%i instances", env->renderSettings->stats.instanceCount);
            ImGui::Text("%i draw calls", env->renderSettings->stats.drawCallCount);
            ImGui::Text("%i shaders", env->renderSettings->stats.shaderCount);
            ImGui::Text("%i materials", env->renderSettings->stats.materialCount);
            ImGui::Text("%i meshes", env->renderSettings->stats.meshCount);
            ImGui::Text("%i lights", env->renderSettings->stats.lightCount);
            ImGui::Text("%i cameras", env->renderSettings->stats.cameraCount);

            env->renderSettings->stats.drawCallCount = 0;
            env->renderSettings->stats.instanceCount = 0;
            env->renderSettings->stats.meshCount = 0;
            env->renderSettings->stats.materialCount = 0;
            env->renderSettings->stats.shaderCount = 0;
            env->renderSettings->stats.lightCount = 0;
            env->renderSettings->stats.cameraCount = 0;
        }
    };

    TRI_STARTUP_CALLBACK("") {
        env->editor->addElement<StatisticsWindow>();
    }

}
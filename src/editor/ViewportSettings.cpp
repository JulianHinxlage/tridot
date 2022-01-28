//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "engine/RuntimeMode.h"
#include <imgui.h>

namespace tri{

    class ViewportSettings : public EditorElement {
    public:
        void startup() override{
            name = "Viewport Settings";
        }

        void update() override{
            bool runtime = env->runtime->getMode() != RuntimeMode::EDIT;
            if(ImGui::Checkbox("play", &runtime)) {
                if (runtime) {
                    env->runtime->setMode(RuntimeMode::RUNTIME);
                } else {
                    env->runtime->setMode(RuntimeMode::EDIT);
                }
            }
            bool pause = env->runtime->getMode() == RuntimeMode::PAUSE;
            if(ImGui::Checkbox("pause", &pause)) {
                if (pause) {
                    env->runtime->setMode(RuntimeMode::PAUSE);
                } else {
                    env->runtime->setMode(RuntimeMode::RUNTIME);
                }
            }

            ImGui::DragFloat("speed", &env->editor->viewport.editorCamera.speed, 4.0, 0.0, 10000.0, "%.3f", ImGuiSliderFlags_Logarithmic);
            std::vector<const char*> items = {
                "Editor Camera",
                "Primary Camera",
                "Fixed Primary Camera",
            };
            ImGui::Combo("camera", (int*)&env->editor->viewport.cameraMode, items.data(), items.size());

            env->editor->gizmos.updateSettings();
        }
    };
    TRI_STARTUP_CALLBACK("") {
        env->editor->addElement<ViewportSettings>();
    }

}
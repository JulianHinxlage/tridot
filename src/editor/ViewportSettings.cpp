//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include <imgui.h>

namespace tri{

    class ViewportSettings : public EditorElement {
    public:
        void startup() override{
            name = "Viewport Settings";
        }

        void update() override{
            bool runtime = env->editor->mode != EDIT;
            if(ImGui::Checkbox("play", &runtime)) {
                if (runtime) {
                    env->editor->setMode(RUNTIME);
                } else {
                    env->editor->setMode(EDIT);
                }
            }
            bool pause = env->editor->mode == PAUSED;
            if(ImGui::Checkbox("pause", &pause)) {
                if(runtime){
                    if (pause) {
                        env->editor->setMode(PAUSED);
                    } else {
                        env->editor->setMode(RUNTIME);
                    }
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
//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Editor.h"
#include "engine/RuntimeMode.h"
#include <imgui/imgui.h>

namespace tri {

    class RuntimeConfigWindow : public EditorElement {
    public:
        std::set<std::string> alwaysOn;
        bool doNotChangeState = false;

        void startup() override {
            name = "Runtime Config";
            type = DEBUG_WINDOW;
            alwaysOn = {
                "Editor",
                "Window",
                "Imgui/begin",
                "Imgui/end",
                "Gizmos/begin",
            };

            env->runtime->setActive("Physics", false, RuntimeMode::EDIT);

            env->signals->runtimeModeChanged.addCallback("Runtime Config", [&]() {
                for (auto& c : alwaysOn) {
                    env->signals->update.setActiveCallback(c, true);
                }
            });
        }

        void update() override {
            auto& observers = env->signals->update.getObservers();
            for (int i = 0; i < observers.size(); i++) {
                auto& observer = observers[i];

                bool active = observer.active;
                bool canChange = alwaysOn.find(observer.name) == alwaysOn.end();
                if (!canChange) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                }
                if (ImGui::Selectable(observer.name.c_str(), &active)) {
                    if (!doNotChangeState && canChange) {
                        env->signals->update.setActiveCallback(observer.name, active);
                        env->runtime->setActive(observer.name, active, env->runtime->getMode());
                    }
                    doNotChangeState = false;
                }
                if (!canChange) {
                    ImGui::PopStyleColor();
                }

                if (canChange && ImGui::IsItemActive() && !ImGui::IsItemHovered()) {
                    int swapIndex = i + (ImGui::GetMouseDragDelta(0).y < 0.0f ? -1.0f : 1.0f);
                    if (swapIndex >= 0 && swapIndex < observers.size()) {
                        env->signals->update.swapObserverPositions(i, swapIndex);
                        ImGui::ResetMouseDragDelta();
                        doNotChangeState = true;
                    }
                }
            }
        }


    };

    TRI_STARTUP_CALLBACK("") {
        env->editor->addElement<RuntimeConfigWindow>();
    }

}

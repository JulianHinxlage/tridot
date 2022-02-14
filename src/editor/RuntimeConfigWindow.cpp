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

        RuntimeMode::Mode mode;
        bool useCurrent;

        void startup() override {
            name = "Runtime Config";
            type = DEBUG_WINDOW;
            useCurrent = true;
            alwaysOn = {
                "Editor",
                "Window",
                "Gui begin",
                "Gui end",
                "Gizmos begin",
                "RenderThread",
            };

            env->runtime->setActive("Physics", false, RuntimeMode::EDIT);

            std::vector<std::string> activeInEdit = {
                "Editor",
                "Window",
                "Gui begin",
                "Gui end",
                "Gizmos begin",
                "RenderThread",

                "HierarchySystem",
                "SignalManager",
                "Console",
                "Reflection",
                "ModuleManager",
                "Profiler",
                "ThreadPool",
                "AssetManager",
                "Camera",
                "Input",
                "MeshComponent",
                "Serializer",
                "Time",
                "Scene",
                "Renderer",
                "Imgui",
                "Skybox",
                "Skybox",
                "Random",
                "RuntimeMode",
                "RenderPipeline",
                "JobSystem",
            };

            for (auto& name : activeInEdit) {
                env->runtime->setActive(name, true, RuntimeMode::EDIT);
            }

            env->signals->runtimeModeChanged.addCallback("Runtime Config", [&]() {
                for (auto& c : alwaysOn) {
                    env->signals->update.setActiveCallback(c, true);
                }
            });
        }

        void update() override {
            TRI_PROFILE("Runtime Config");
            auto& observers = env->signals->update.getObservers();

            //mode selection
            std::string preview;
            if (useCurrent) {
                preview = "CURRENT";
            }
            else {
                if (mode == RuntimeMode::EDIT) {
                    preview = "EDIT";
                }else if (mode == RuntimeMode::RUNTIME) {
                    preview = "RUNTIME";
                }else if (mode == RuntimeMode::PAUSE) {
                    preview = "PAUSE";
                }
            }
            if (ImGui::BeginCombo("mode", preview.c_str())) {
                if (ImGui::Selectable("CURRENT", useCurrent)) {
                    useCurrent = true;
                }
                if (ImGui::Selectable("EDIT", mode == RuntimeMode::EDIT && !useCurrent)) {
                    useCurrent = false;
                    mode = RuntimeMode::EDIT;
                }
                if (ImGui::Selectable("RUNTIME", mode == RuntimeMode::RUNTIME && !useCurrent)) {
                    useCurrent = false;
                    mode = RuntimeMode::RUNTIME;
                }
                if (ImGui::Selectable("PAUSE", mode == RuntimeMode::PAUSE && !useCurrent)) {
                    useCurrent = false;
                    mode = RuntimeMode::PAUSE;
                }
                ImGui::EndCombo();
            }

            ImGui::Separator();

            if (useCurrent) {
                mode = env->runtime->getMode();
            }

            //show callbacks
            for (int i = 0; i < observers.size(); i++) {
                auto& observer = observers[i];
                bool active = env->runtime->getActive(observer.name, mode);
                bool canChange = alwaysOn.find(observer.name) == alwaysOn.end();
                if (!canChange) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                }
                if (ImGui::Selectable(observer.name.c_str(), &active)) {
                    if (!doNotChangeState && canChange) {
                        if (mode == env->runtime->getMode()) {
                            env->signals->update.setActiveCallback(observer.name, active);
                        }
                        env->runtime->setActive(observer.name, active, mode);
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

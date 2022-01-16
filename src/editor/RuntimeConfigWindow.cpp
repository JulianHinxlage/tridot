//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Editor.h"
#include <imgui/imgui.h>

namespace tri {

    class RuntimeConfigWindow : public EditorElement {
    public:
        std::set<std::string> alwaysOn;
        bool doNotChangeState = false;
        std::set<std::string> editModeConfig;
        std::set<std::string> runtimeModeConfig;
        RuntimeMode currentMode = RUNTIME;

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

            /*
            editModeConfig = {
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
            };
            */

            env->signals->update.setActiveInEditModeCallback("Physics", false);

            for (auto& observer : env->signals->update.getObservers()) {
                if (observer.activeInEditMode) {
                    editModeConfig.insert(observer.name);
                }
            }

            for(auto &observer : env->signals->update.getObservers()){
                if(observer.active){
                    runtimeModeConfig.insert(observer.name);
                }
            }

            env->signals->preUpdate.addCallback("RuntimeConfig", [&](){
                modeChange(env->editor->mode);
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

                        if (active) {
                            if (currentMode == EDIT) {
                                editModeConfig.insert(observer.name);
                            }
                            else {
                                runtimeModeConfig.insert(observer.name);
                            }
                        }
                        else {
                            if (currentMode == EDIT) {
                                editModeConfig.erase(observer.name);
                            }
                            else {
                                runtimeModeConfig.erase(observer.name);
                            }
                        }

                    }
                    doNotChangeState = false;
                }
                if (!canChange) {
                    ImGui::PopStyleColor();
                }

                if (canChange) {
                    if (currentMode == EDIT) {
                        bool contains = editModeConfig.find(observer.name) != editModeConfig.end();
                        if (active != contains) {
                            env->signals->update.setActiveCallback(observer.name, contains);
                            if (active) {
                                runtimeModeConfig.insert(observer.name);
                            }
                            else {
                                runtimeModeConfig.erase(observer.name);
                            }
                        }
                    }
                    else {
                        bool contains = runtimeModeConfig.find(observer.name) != runtimeModeConfig.end();
                        if (active != contains) {
                            if (active) {
                                runtimeModeConfig.insert(observer.name);
                            }
                            else {
                                runtimeModeConfig.erase(observer.name);
                            }
                        }
                    }
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

        void modeChange(RuntimeMode mode) {
            if(currentMode != mode) {
                if (mode == RUNTIME) {

                    env->signals->update.setActiveAll(false);
                    for (auto &name : alwaysOn) {
                        env->signals->update.setActiveCallback(name, true);
                    }
                    for (auto &name : runtimeModeConfig) {
                        env->signals->update.setActiveCallback(name, true);
                    }

                } else if (mode == EDIT || mode == PAUSED) {

                    env->signals->update.setActiveAll(false);
                    for (auto &name : alwaysOn) {
                        env->signals->update.setActiveCallback(name, true);
                    }
                    for (auto &name : editModeConfig) {
                        env->signals->update.setActiveCallback(name, true);
                    }

                }
                currentMode = mode;
            }
        }

    };

    TRI_STARTUP_CALLBACK("") {
        env->editor->addElement<RuntimeConfigWindow>();
    }

}

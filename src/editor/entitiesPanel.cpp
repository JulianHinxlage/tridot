//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/engine/Engine.h"
#include "Editor.h"
#include <imgui.h>

using namespace tridot;
using namespace ecs;

TRI_UPDATE("panels"){
    if(ImGui::GetCurrentContext() != nullptr) {
        bool &open = Editor::getFlag("Entities");
        if(open) {
            if (ImGui::Begin("Entities", &open)) {
                ImGui::SetWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);

                if (ImGui::Button("add Entity")) {
                    Editor::selectedEntity = engine.create();
                }

                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
                    if (engine.input.pressed(engine.input.KEY_DELETE)) {
                        if (Editor::selectedEntity != -1) {
                            engine.destroy(Editor::selectedEntity);
                            Editor::selectedEntity = -1;
                        }
                    }
                }

                ImGui::Separator();
                ImGui::BeginChild("entities");


                EntityId max = 0;
                engine.view<>().each([&](EntityId id) {
                    max = std::max(max, id);
                });
                for (EntityId id = 0; id <= max; id++) {
                    if (engine.exists(id)) {
                        std::string label = "Entity " + std::to_string(id);

                        if(id == Editor::cameraId){
                            label = "<Editor Camera>";
                        }

                        if (ImGui::Selectable(label.c_str(), id == Editor::selectedEntity,
                                              ImGuiSelectableFlags_AllowItemOverlap)) {
                            if (Editor::selectedEntity == id) {
                                Editor::selectedEntity = -1;
                            } else {
                                Editor::selectedEntity = id;
                            }
                        }

                        ImGui::PushID(id);
                        if (ImGui::BeginPopupContextItem()) {
                            if (ImGui::Selectable("remove")) {
                                engine.destroy(id);
                                if (id == Editor::selectedEntity) {
                                    Editor::selectedEntity = -1;
                                }
                            }
                            ImGui::EndPopup();
                        }
                        ImGui::PopID();
                    }
                }

                ImGui::EndChild();
            }
            ImGui::End();
        }
    }
}

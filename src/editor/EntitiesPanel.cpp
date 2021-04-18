//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "EntitiesPanel.h"
#include "tridot/engine/Engine.h"
#include "tridot/components/Tag.h"
#include "Editor.h"
#include <imgui.h>

using namespace ecs;

namespace tridot {

    void EntitiesPanel::update(){
        if (ImGui::GetCurrentContext() != nullptr) {
            bool& open = Editor::getFlag("Entities");
            if (open) {
                if (ImGui::Begin("Entities", &open)) {
                    ImGui::SetWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);

                    bool controlDown = engine.input.down(Input::KEY_RIGHT_CONTROL) || engine.input.down(Input::KEY_LEFT_CONTROL);
                    bool shiftDown = engine.input.down(Input::KEY_RIGHT_SHIFT) || engine.input.down(Input::KEY_LEFT_SHIFT);

                    if (ImGui::Button("add Entity")) {
                        Editor::selection.select(engine.create(Transform(), Tag(), uuid()));
                    }

                    if (ImGui::IsWindowHovered(ImGuiFocusedFlags_ChildWindows)) {
                        if (controlDown) {
                            if (engine.input.pressed('D')) {
                                Editor::selection.duplicateAll();
                            }
                        }
                        if (engine.input.pressed(engine.input.KEY_DELETE)) {
                            Editor::selection.destroyAll();
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
                            ImGui::PushID(id);

                            std::string label = "";
                            if (engine.has<Tag>(id)) {
                                label = engine.get<Tag>(id).tag;
                            }
                            if (label.empty()) {
                                label = "Entity " + std::to_string(id);
                            }

                            if (ImGui::Selectable(label.c_str(), Editor::selection.isSelected(id))) {
                                if (shiftDown) {
                                    ecs::EntityId min = std::min(id, Editor::selection.lastSelected);
                                    ecs::EntityId max = std::max(id, Editor::selection.lastSelected);
                                    for (EntityId id = min; id <= max; id++) {
                                        if (engine.exists(id)) {
                                            Editor::selection.select(id, false);
                                        }
                                    }
                                }
                                else {
                                    if (Editor::selection.isSelected(id)) {
                                        if (controlDown) {
                                            Editor::selection.unselect(id);
                                        }
                                        else {
                                            Editor::selection.unselect();
                                        }
                                    }
                                    else {
                                        Editor::selection.select(id, !controlDown);
                                    }
                                }
                            }

                            if (ImGui::BeginPopupContextItem()) {
                                if (ImGui::Selectable("remove")) {
                                    if (Editor::selection.isSelected(id)) {
                                        Editor::selection.destroyAll();
                                    }
                                    else {
                                        Editor::undo.destroyEntity(id);
                                        engine.destroy(id);
                                        if (Editor::selection.isSelected(id)) {
                                            Editor::selection.unselect(id);
                                        }
                                    }
                                }
                                if (ImGui::Selectable("duplicate")) {
                                    if (Editor::selection.isSelected(id)) {
                                        Editor::selection.duplicateAll();
                                    }
                                    else {
                                        Editor::selection.select(Editor::selection.duplicate(id));
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

}
//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/engine/Engine.h"
#include "Editor.h"
#include "EditorGui.h"
#include <imgui.h>

using namespace tridot;
using namespace ecs;

TRI_UPDATE("panels"){
    if(ImGui::GetCurrentContext() != nullptr) {
        bool &open = Editor::getFlag("Properties");
        if(open) {
            if (ImGui::Begin("Properties", &open)) {
                ImGui::SetWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
                EntityId id = Editor::selectedEntity;
                if (id != -1 && engine.exists(id)) {

                    auto &types = ecs::Reflection::getTypes();
                    if (ImGui::Button("add Component")) {
                        ImGui::OpenPopup("add");
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("remove Entity")) {
                        engine.destroy(id);
                        Editor::selectedEntity = -1;
                    }
                    if (ImGui::BeginPopup("add")) {
                        for (auto &type : types) {
                            if(type) {
                                auto *pool = engine.getPool(type->id());
                                if (pool && !pool->has(id)) {
                                    if (ImGui::Button(type->name().c_str())) {
                                        pool->add(id, nullptr);
                                        ImGui::CloseCurrentPopup();
                                    }
                                }
                            }
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::Separator();
                    ImGui::BeginChild("properties", ImVec2(0, 0), false, Editor::propertiesWindowFlags);
                    Editor::propertiesWindowFlags = 0;

                    for (auto &type : types) {
                        if(type){
                            if(engine.has(id, type->id())){
                                void *comp = engine.get(id, type->id());
                                bool open = true;
                                if (ImGui::CollapsingHeader(type->name().c_str(), &open, ImGuiTreeNodeFlags_DefaultOpen)) {
                                    EditorGui::drawType(type->id(), comp, type->name());
                                }
                                if (!open) {
                                    engine.remove(id, type->id());
                                }
                            }
                        }
                    }

                    ImGui::EndChild();
                }
            }
            ImGui::End();
        }
    }
}

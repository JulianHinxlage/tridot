//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/engine/Engine.h"
#include "Editor.h"
#include "EditorGui.h"
#include <imgui.h>

using namespace tridot;
using namespace ecs;

void updateProperties(ecs::EntityId id);
void updateMultiProperties();

TRI_UPDATE("panels"){
    if(ImGui::GetCurrentContext() != nullptr) {
        bool &open = Editor::getFlag("Properties");
        if(open) {
            if (ImGui::Begin("Properties", &open)) {
                ImGui::SetWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
                EntityId id = Editor::selection.getSingleSelection();
                if (id != -1 && engine.exists(id)) {
                    //single entity
                    updateProperties(id);
                } else if(Editor::selection.selectedEntities.size() > 1){
                    //multiple entities
                    updateMultiProperties();
                }
            }
            ImGui::End();
        }
    }
}

void updateProperties(ecs::EntityId id) {
    auto& types = ecs::Reflection::getTypes();
    if (ImGui::Button("add Component")) {
        ImGui::OpenPopup("add");
    }
    ImGui::SameLine();
    if (ImGui::Button("remove Entity")) {
        Editor::selection.destroyAll();
    }
    if (ImGui::BeginPopup("add")) {
        for (auto& type : types) {
            if (type) {
                auto* pool = engine.getPool(type->id());
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

    for (auto& type : types) {
        if (type) {
            if (engine.has(id, type->id())) {
                void* comp = engine.get(id, type->id());
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

bool getDifferences(ecs::Reflection::Type* type, void* v1, void* v2, std::vector<std::pair<int, ecs::Reflection::Type*>> &differences, int currentOffset = 0) {
    for (auto& member : type->member()) {
        currentOffset += member.offset;
        getDifferences(&ecs::Reflection::get(member.typeId), (uint8_t*)v1 + member.offset, (uint8_t*)v2 + member.offset, differences, currentOffset);
        currentOffset -= member.offset;
    }
    if (type->member().size() == 0) {
        if (!type->equals(v1, v2)) {
            differences.push_back({ currentOffset, type });
        }
    }
    return differences.size() > 0;
}

void updateMultiProperties() {
    auto& types = ecs::Reflection::getTypes();
    if (ImGui::Button("add Component")) {
        ImGui::OpenPopup("add");
    }
    ImGui::SameLine();
    if (ImGui::Button("remove Entity")) {
        Editor::selection.destroyAll();
    }
    if (ImGui::BeginPopup("add")) {
        for (auto& type : types) {
            if (type) {
                bool show = false;
                for (auto& sel : Editor::selection.selectedEntities) {
                    ecs::EntityId id = sel.first;
                    auto* pool = engine.getPool(type->id());
                    if (pool && !pool->has(id)) {
                        show = true;
                    }
                }

                if (show) {
                    if (ImGui::Button(type->name().c_str())) {

                        for (auto& sel : Editor::selection.selectedEntities) {
                            ecs::EntityId id = sel.first;
                            auto* pool = engine.getPool(type->id());
                            if (pool && !pool->has(id)) {
                                pool->add(id, nullptr);
                                ImGui::CloseCurrentPopup();
                            }
                        }

                    }
                }
            }
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();
    ImGui::BeginChild("properties", ImVec2(0, 0), false, Editor::propertiesWindowFlags);
    Editor::propertiesWindowFlags = 0;

    for (auto& type : types) {
        if (type) {

            bool first = true;
            bool remove = false;
            bool change = false;
            std::vector<std::pair<int, ecs::Reflection::Type*>> differences;
            void* compBuffer = new uint8_t[type->size()];

            for (auto& sel : Editor::selection.selectedEntities) {
                ecs::EntityId id = sel.first;

                if (engine.has(id, type->id())) {
                    void* comp = engine.get(id, type->id());

                    if (first) {
                        type->copy(comp, compBuffer);
                        bool open = true;
                        if (ImGui::CollapsingHeader(type->name().c_str(), &open, ImGuiTreeNodeFlags_DefaultOpen)) {
                            EditorGui::drawType(type->id(), comp, type->name());
                        }
                        if (!open) {
                            remove = true;
                        }
                        if (getDifferences(type, comp, compBuffer, differences)) {
                            change = true;
                            type->copy(comp, compBuffer);
                        }
                    }

                    if (change && !first) {
                        for (auto difference : differences) {
                            difference.second->copy((uint8_t*)compBuffer + difference.first, (uint8_t*)comp + difference.first);
                        }
                    }
                    if (remove) {
                        engine.remove(id, type->id());
                    }

                    first = false;
                }
            }

            delete[] compBuffer;

        }
    }
    ImGui::EndChild();
}
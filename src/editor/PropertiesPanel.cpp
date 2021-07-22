//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "PropertiesPanel.h"
#include "Editor.h"
#include "EditorGui.h"
#include <imgui/imgui.h>

using namespace tridot;

namespace tridot {

	void PropertiesPanel::update(){
        EditorGui::window("Properties", [this](){
            EntityId id = env->editor->selection.getSingleSelection();
            if (id != -1 && env->scene->exists(id)) {
                //single entity
                updateProperties(id);
            }
            else if (env->editor->selection.entities.size() > 1) {
                //multiple entities
                updateMultiProperties();
            }
        });
	}

    bool getDifferences(Reflection::TypeDescriptor* type, void* v1, void* v2, std::vector<std::pair<int, Reflection::TypeDescriptor*>>& differences, int currentOffset = 0) {
        for (auto& member : type->member()) {
            currentOffset += member.offset;
            getDifferences(member.descriptor, (uint8_t*)v1 + member.offset, (uint8_t*)v2 + member.offset, differences, currentOffset);
            currentOffset -= member.offset;
        }
        if (type->member().size() == 0) {
            if (!type->equals(v1, v2)) {
                differences.push_back({ currentOffset, type });
            }
        }
        return differences.size() > 0;
    }

	void PropertiesPanel::updateProperties(EntityId id){
        auto& types = env->reflection->getDescriptors();
        if (ImGui::Button("add Component")) {
            ImGui::OpenPopup("add");
        }
        ImGui::SameLine();
        if (ImGui::Button("remove Entity")) {
            env->editor->selection.destroyAll();
        }
        if (ImGui::BeginPopup("add")) {
            for (auto& type : types) {
                if (type) {
                    auto* pool = env->scene->getPool(type->id());
                    if (pool && !pool->has(id)) {
                        if (ImGui::Button(type->name().c_str())) {
                            pool->add(id, nullptr);
                            env->editor->undo.beginAction();
                            env->editor->undo.addCustomAction([id, rid = type->id()](){
                                env->scene->remove(id, rid);
                            }, [id, rid = type->id()](){
                                env->scene->addByTypeId(id, rid);
                            });
                            env->editor->undo.endAction();
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
            }
            ImGui::EndPopup();
        }

        ImGui::Separator();
        ImGui::BeginChild("properties", ImVec2(0, 0), false, env->editor->propertiesWindowFlags);
        env->editor->propertiesWindowFlags = 0;

        static Reflection::TypeDescriptor *lastChange;

        for (auto& type : types) {
            if (type) {
                if (env->scene->has(id, type->id())) {
                    void* comp = env->scene->get(id, type->id());
                    bool open = true;
                    if (ImGui::CollapsingHeader(type->name().c_str(), &open, ImGuiTreeNodeFlags_DefaultOpen)) {
                        uint8_t* compBuffer = new uint8_t[type->size()];
                        type->copy(comp, compBuffer);

                        EditorGui::drawType(type->id(), comp, type->name());

                        std::vector<std::pair<int, Reflection::TypeDescriptor*>> differences;
                        if(getDifferences(type, comp, compBuffer,differences)){
                            env->editor->undo.changeComponent(id, type, compBuffer);
                            env->editor->undo.changeComponent(id, type, comp);
                            lastChange = type;
                        }else{
                            if(type == lastChange){
                                lastChange = nullptr;
                                env->editor->undo.endAction();
                            }
                        }
                        delete[] compBuffer;
                    }
                    if (!open) {
                        env->editor->undo.beginAction();
                        env->editor->undo.changeComponent(id, type, comp);
                        env->editor->undo.addCustomAction(nullptr, [id, rid = type->id()](){
                            env->scene->remove(id, rid);
                        });
                        env->editor->undo.endAction();
                        env->scene->remove(id, type->id());
                    }
                }
            }
        }

        ImGui::EndChild();
	}

	void PropertiesPanel::updateMultiProperties(){
        auto& types = env->reflection->getDescriptors();
        if (ImGui::Button("add Component")) {
            ImGui::OpenPopup("add");
        }
        ImGui::SameLine();
        if (ImGui::Button("remove Entity")) {
            env->editor->selection.destroyAll();
        }
        if (ImGui::BeginPopup("add")) {
            for (auto& type : types) {
                if (type) {
                    bool show = false;
                    for (auto& sel : env->editor->selection.entities) {
                        EntityId id = sel.first;
                        auto* pool = env->scene->getPool(type->id());
                        if (pool && !pool->has(id)) {
                            show = true;
                        }
                    }

                    if (show) {
                        if (ImGui::Button(type->name().c_str())) {

                            for (auto& sel : env->editor->selection.entities) {
                                EntityId id = sel.first;
                                auto* pool = env->scene->getPool(type->id());
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
        ImGui::BeginChild("properties", ImVec2(0, 0), false, env->editor->propertiesWindowFlags);
        env->editor->propertiesWindowFlags = 0;

        for (auto& type : types) {
            if (type) {

                bool first = true;
                bool remove = false;
                bool change = false;
                static Reflection::TypeDescriptor *lastChange;
                std::vector<std::pair<int, Reflection::TypeDescriptor*>> differences;
                uint8_t* compBuffer = new uint8_t[type->size()];

                for (auto& sel : env->editor->selection.entities) {
                    EntityId id = sel.first;

                    if (env->scene->has(id, type->id())) {
                        void* comp = env->scene->get(id, type->id());

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
                                env->editor->undo.changeComponent(id, type, compBuffer);
                                change = true;
                                type->copy(comp, compBuffer);
                                env->editor->undo.changeComponent(id, type, comp);
                            }
                        }

                        if (change && !first) {
                            for (auto difference : differences) {
                                env->editor->undo.changeComponent(id, type, comp);
                                difference.second->copy((uint8_t*)compBuffer + difference.first, (uint8_t*)comp + difference.first);
                                env->editor->undo.changeComponent(id, type, comp);
                            }
                        }
                        if (remove) {
                            env->scene->remove(id, type->id());
                        }

                        first = false;
                    }
                }

                if(!change && lastChange == type){
                    env->editor->undo.endAction();
                    lastChange = nullptr;
                }
                if(change){
                    lastChange = type;
                }

                delete[] compBuffer;
            }
        }
        ImGui::EndChild();
	}

}

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "EntitiesPanel.h"
#include "tridot/engine/Input.h"
#include "tridot/components/Tag.h"
#include "Editor.h"
#include <imgui.h>

using namespace tridot;

namespace tridot {

    bool isParentOf(EntityId id, EntityId parentId){
        if(env->scene->has<Transform>(id)){
            auto &t = env->scene->get<Transform>(id);
            if(t.parent.id != -1) {
                if(t.parent.id == parentId){
                    return true;
                }else {
                    return isParentOf(t.parent.id, parentId);
                }
            }
        }
        return false;
    }

    void EntitiesPanel::updateEntityMenu(EntityId id){
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::Selectable("remove")) {
                if (env->editor->selection.isSelected(id)) {
                    env->editor->selection.destroyAll();
                }
                else {
                    env->editor->undo.destroyEntity(id);
                    env->scene->destroy(id);
                    if (env->editor->selection.isSelected(id)) {
                        env->editor->selection.unselect(id);
                    }
                }
            }
            if (ImGui::Selectable("duplicate")) {
                if (env->editor->selection.isSelected(id)) {
                    env->editor->selection.duplicateAll();
                }
                else {
                    env->editor->selection.select(env->editor->selection.duplicate(id));
                }
            }
            if (ImGui::Selectable("parent")) {
                if(env->scene->has<Transform>(id)){
                    env->editor->undo.beginAction();
                    Transform &parentTransform = env->scene->get<Transform>(id);
                    for(auto &sel : env->editor->selection.entities){
                        if(env->scene->has<Transform>(sel.first)){
                            Transform &transform = env->scene->get<Transform>(sel.first);
                            env->editor->undo.changeComponent(sel.first, env->reflection->getDescriptor<Transform>(), &transform);
                            transform.decompose(glm::inverse(parentTransform.getMatrix()) * transform.getMatrix());
                            transform.parent.id = id;
                            env->editor->undo.changeComponent(sel.first, env->reflection->getDescriptor<Transform>(), &transform);
                        }
                    }
                    env->editor->undo.endAction();
                }
            }
            if (ImGui::Selectable("unparent")) {
                if (env->editor->selection.isSelected(id)) {
                    env->editor->undo.beginAction();
                    for(auto &sel : env->editor->selection.entities){
                        EntityId id = sel.first;
                        if(env->scene->has<Transform>(id)){
                            Transform &transform = env->scene->get<Transform>(id);
                            env->editor->undo.changeComponent(sel.first, env->reflection->getDescriptor<Transform>(), &transform);
                            transform.decompose(transform.getMatrix());
                            transform.parent.id = -1;
                            env->editor->undo.changeComponent(sel.first, env->reflection->getDescriptor<Transform>(), &transform);
                        }
                    }
                    env->editor->undo.endAction();
                }
                else {
                    if(env->scene->has<Transform>(id)){
                        env->editor->undo.beginAction();
                        Transform &transform = env->scene->get<Transform>(id);
                        env->editor->undo.changeComponent(id, env->reflection->getDescriptor<Transform>(), &transform);
                        transform.decompose(transform.getMatrix());
                        transform.parent.id = -1;
                        env->editor->undo.changeComponent(id, env->reflection->getDescriptor<Transform>(), &transform);
                        env->editor->undo.endAction();
                    }
                }
            }
            if(ImGui::Selectable("add child")){
                newEntity(id);
            }
            ImGui::EndPopup();
        }
    }

    void EntitiesPanel::updateEntity(EntityId id, bool controlDown, bool shiftDown){
        std::string label = "";
        if (env->scene->has<Tag>(id)) {
            label = env->scene->get<Tag>(id).tag;
        }
        if (label.empty()) {
            label = "Entity " + std::to_string(id);
        }

        bool hasChildren = Transform::hasChildren(id);
        ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_SpanAvailWidth |
                ImGuiTreeNodeFlags_SpanFullWidth;
        if(env->editor->selection.isSelected(id)){
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        if(!hasChildren){
            flags |= ImGuiTreeNodeFlags_Leaf;
        }
        bool treeOpen = ImGui::TreeNodeEx(label.c_str(), flags);

        if(ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax())){
            hoveredEntity = id;
        }

        if(ImGui::IsItemClicked() && treeOpen == treesOpen[id]){
            clickedEntity = id;
            if (shiftDown) {
                auto &pool = env->scene->getEntityPool();
                int min = pool.getIndex(id);
                int max = pool.getIndex(env->editor->selection.lastSelected);
                if(min > max){
                    std::swap(min, max);
                }
                for(int i = min; i <= max; i++){
                    EntityId id = pool.getId(i);
                    if (env->scene->exists(id)) {
                        env->editor->selection.select(id, false);
                    }
                }
            }
            else {
                if (env->editor->selection.isSelected(id)) {
                    if (controlDown) {
                        env->editor->selection.unselect(id);
                    }
                    else {
                        env->editor->selection.unselect();
                    }
                }
                else {
                    env->editor->selection.select(id, !controlDown);
                }
            }
        }

        updateEntityMenu(id);

        if(hasChildren && treeOpen){
            for(auto &child : Transform::getChildren(id)){
                ImGui::PushID(child);
                updateEntity(child, controlDown, shiftDown);
                ImGui::PopID();
            }
        }

        treesOpen[id] = treeOpen;
        if(treeOpen){
            ImGui::TreePop();
        }
    }

    void EntitiesPanel::update(){
        if(env->input->pressed(Input::MOUSE_BUTTON_LEFT)){
            clickedEntity = -1;
            hoveredEntity = -1;
        }
        EditorGui::window("Entities", [this](){
            bool controlDown = env->input->down(Input::KEY_RIGHT_CONTROL) || env->input->down(Input::KEY_LEFT_CONTROL);
            bool shiftDown = env->input->down(Input::KEY_RIGHT_SHIFT) || env->input->down(Input::KEY_LEFT_SHIFT);

            if (ImGui::Button("add Entity")) {
                newEntity();
            }

            if (ImGui::IsWindowHovered(ImGuiFocusedFlags_ChildWindows)) {
                if (controlDown) {
                    if (env->input->pressed('D')) {
                        env->editor->selection.duplicateAll();
                    }
                }
                if (env->input->pressed(env->input->KEY_DELETE)) {
                    env->editor->selection.destroyAll();
                }
            }

            ImGui::Separator();
            ImGui::BeginChild("entities");

            auto &pool = env->scene->getEntityPool();
            for(int i = 0; i < pool.getEntities().size(); i++){
                EntityId id = pool.getId(i);
                bool root = false;
                if(env->scene->has<Transform>(id)){
                    Transform &t = env->scene->get<Transform>(id);
                    if(t.parent.id == -1){
                        root = true;
                    }
                }else {
                    root = true;
                }
                if(root){
                    ImGui::PushID(id);
                    updateEntity(id, controlDown, shiftDown);
                    ImGui::PopID();
                }
            }

            if(env->input->down(Input::MOUSE_BUTTON_LEFT)){
                if(clickedEntity != -1 && hoveredEntity != -1){
                    if(clickedEntity != hoveredEntity){
                        if(!isParentOf(hoveredEntity, clickedEntity)) {
                            int i1 = pool.getIndex(clickedEntity);
                            int i2 = pool.getIndex(hoveredEntity);
                            if (i2 > i1) {
                                for (int i = i1 + 1; i <= i2; i++) {
                                    pool.swap(i, i - 1);
                                }
                            } else if (i1 > i2) {
                                for (int i = i1 - 1; i >= i2; i--) {
                                    pool.swap(i, i + 1);
                                }
                            }
                        }
                    }
                }
            }

            ImGui::EndChild();
        });
    }

    void EntitiesPanel::newEntity(EntityId parentId, bool addAction) {
        Transform transform;
        transform.parent.id = parentId;
        EntityId id = env->scene->create(transform, Tag(), uuid());
        env->editor->selection.select(id);

        if(addAction) {
            env->editor->undo.beginAction();
            env->editor->undo.addCustomAction([id]() {
                env->scene->destroy(id);
            }, [parentId, this]() {
                newEntity(parentId, false);
            });
            env->editor->undo.endAction();
        }
    }

}
//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "entity/Scene.h"
#include "engine/Input.h"
#include "engine/EntityInfo.h"
#include <imgui.h>

namespace tri {

    class EntitiesWindow : public EditorWindow {
    public:
        EntityId active;
        EntityId hovered;
        EntityId lastClicked;
        bool lastClickedWasSelected;
        bool blockStateChange;
        bool control;
        bool shift;
        std::vector<EntityId> entityOrder;
        std::vector<EntityId> lastFrameEntityOrder;

        void startup() {
            name = "Entities";
            active = -1;
            hovered = -1;
            lastClicked = -1;
            lastClickedWasSelected = false;
            blockStateChange = false;
            control = false;
            shift = false;

            env->signals->sceneLoad.addCallback([&](Scene *scene){
                active = -1;
                hovered = -1;
                lastClicked = -1;
                lastClickedWasSelected = false;
                blockStateChange = false;
            });
        }

        void update() override {
            control = env->input->down(Input::KEY_LEFT_CONTROL) || env->input->down(Input::KEY_RIGHT_CONTROL);
            shift = env->input->down(Input::KEY_LEFT_SHIFT) || env->input->down(Input::KEY_RIGHT_SHIFT);

            updateHeader();

            ImGui::Separator();
            ImGui::BeginChild("");

            hovered = -1;
            entityOrder.clear();

            env->scene->view<>().each([&](EntityId id){
                if(env->scene->hasComponent<Transform>(id)){
                    Transform &t = env->scene->getComponent<Transform>(id);
                    if(t.parent == -1){
                        if(updateEntity(id)){
                            updateEntityChildren(id);
                            ImGui::TreePop();
                        }
                    }
                }else{
                    if(updateEntity(id)){
                        updateEntityChildren(id);
                        ImGui::TreePop();
                    }
                }
            });

            updateEntityDragging();
            updateShortcuts();
            updateMenu();

            lastFrameEntityOrder = entityOrder;

            ImGui::EndChild();
        }

        void updateEntityChildren(EntityId id){
            for(auto child : env->systems->getSystem<TransformSystem>()->getChildren(id)){
                if(updateEntity(child)){
                    updateEntityChildren(child);
                    ImGui::TreePop();
                }
            }
        }

        bool updateEntity(EntityId id){
            entityOrder.push_back(id);
            ImGui::PushID(id);
            std::string label = "";
            if(env->scene->hasComponent<EntityInfo>(id)){
                label = env->scene->getComponent<EntityInfo>(id).name;
            }
            if(label.empty()){
                label = "<entity " + std::to_string(id) + ">";
            }

            bool selected = env->editor->selectionContext.isSelected(id);
            bool hasChildren = env->systems->getSystem<TransformSystem>()->getChildren(id).size() > 0;

            ImGuiTreeNodeFlags flags = 0;
            if(!hasChildren){
                flags |= ImGuiTreeNodeFlags_Leaf;
            }
            bool open = ImGui::TreeNodeEx("", flags);
            ImGui::SameLine();
            if(ImGui::Selectable(label.c_str(), selected)){
                if(!blockStateChange){
                    clicked(id);
                }
            }

            if(ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax())){
                hovered = id;
            }

            updateEntityMenu(id);
            ImGui::PopID();
            return open;
        }

        void updateHeader(){
            if(ImGui::Button("Add Entity")){
                EntityId id = env->editor->entityOperations.addEntity();
                env->scene->addComponent<EntityInfo>(id);
                env->editor->selectionContext.unselectAll();
                env->editor->selectionContext.select(id);
            }
        }

        void updateShortcuts(){
            if(control){
                if(env->input->pressed("D")){
                    env->editor->entityOperations.duplicateSelection();
                }
                if(env->input->pressed("C")){
                    if(env->editor->selectionContext.getSelected().size() == 1) {
                        for (auto &id : env->editor->selectionContext.getSelected()) {
                            env->editor->entityOperations.copyEntity(id);
                        }
                    }else{
                        env->console->warning("cannot copy multiple entities");
                    }
                }
                if(env->input->pressed("V")){
                    if(env->editor->entityOperations.wasEntityCopied()) {
                        EntityId copy = env->editor->entityOperations.pastEntity();
                        env->editor->selectionContext.unselectAll();
                        env->editor->selectionContext.select(copy);
                    }
                }
            }
            if(env->input->pressed(Input::KEY_DELETE)){
                env->editor->entityOperations.removeSelection();
            }
        }

        void updateEntityMenu(EntityId id){
            if(ImGui::BeginPopupContextItem()){
                if(ImGui::MenuItem("Delete", "Delete")){
                    if(env->editor->selectionContext.isSelected(id)){
                        env->editor->entityOperations.removeSelection();
                    }else{
                        env->editor->entityOperations.removeEntity(id);
                        env->editor->selectionContext.unselect(id);
                    }
                }
                if(ImGui::MenuItem("Copy", "Ctrl+C")){
                    env->editor->entityOperations.copyEntity(id);
                }
                if(ImGui::MenuItem("Past", "Ctrl+V", false, env->editor->entityOperations.wasEntityCopied())){
                    EntityId copy = env->editor->entityOperations.pastEntity();
                    env->editor->selectionContext.unselectAll();
                    env->editor->selectionContext.select(copy);
                }
                if(ImGui::MenuItem("Duplicate", "Ctrl+D")){
                    if(env->editor->selectionContext.isSelected(id)){
                        env->editor->entityOperations.duplicateSelection();
                    }else{
                        EntityId copy = env->editor->entityOperations.duplicateEntity(id);
                        env->editor->selectionContext.unselectAll();
                        env->editor->selectionContext.select(copy);
                    }
                }
                if(ImGui::MenuItem("Parent", nullptr, false, env->editor->selectionContext.getSelected().size() > 0)){
                    for (auto &id2 : env->editor->selectionContext.getSelected()) {
                        if(id != id2){
                            if(env->scene->hasComponent<Transform>(id2)){
                                Transform &t2 = env->scene->getComponent<Transform>(id2);
                                Transform preEdit = t2;
                                t2.parent = id;
                                if(env->scene->hasComponent<Transform>(id)){
                                    Transform &t = env->scene->getComponent<Transform>(id);
                                    t2.decompose(glm::inverse(t.getMatrix()) * t2.getMatrix());
                                }
                                env->editor->undo.componentChanged(env->reflection->getTypeId<Transform>(), id2, &preEdit);
                            }
                        }
                    }
                }
                if(env->scene->hasComponent<Transform>(id)){
                    Transform &t = env->scene->getComponent<Transform>(id);
                    if(t.parent != -1){
                        if(ImGui::MenuItem("Unparent")){
                            Transform preEdit = t;
                            t.parent = -1;
                            t.decompose(t.getMatrix());
                            env->editor->undo.componentChanged(env->reflection->getTypeId<Transform>(), id, &preEdit);
                        }
                    }
                }

                ImGui::EndPopup();
            }
        }

        void updateMenu(){
            if (ImGui::BeginPopupContextWindow("entity", ImGuiMouseButton_Right, false)) {
                if (ImGui::MenuItem("Past", "Ctrl+V", false, env->editor->entityOperations.wasEntityCopied())) {
                    EntityId copy = env->editor->entityOperations.pastEntity();
                    env->editor->selectionContext.unselectAll();
                    env->editor->selectionContext.select(copy);
                }
                ImGui::EndPopup();
            }
        }

        void updateEntityDragging(){
            if(active != -1 && hovered != -1){
                if(hovered != active){

                    if(env->systems->getSystem<TransformSystem>()->haveSameParent(active, hovered)) {
                        auto *pool = env->scene->getEntityPool();
                        while (true) {

                            int index = -1;
                            int targetIndex = -1;
                            for (int i = 0; i < entityOrder.size(); i++) {
                                EntityId id = entityOrder[i];
                                if (active == id) {
                                    index = i;
                                }
                                if (hovered == id) {
                                    targetIndex = i;
                                }
                            }
                            if (index == targetIndex) {
                                break;
                            }

                            if (index != -1 && targetIndex != -1) {
                                if (targetIndex > index) {
                                    int swapIndex = pool->getIndexById(entityOrder[index]);
                                    int swapTargetIndex = pool->getIndexById(entityOrder[index + 1]);
                                    pool->swap(swapIndex, swapTargetIndex);

                                    auto *tpool = env->scene->getComponentPool<Transform>();
                                    if(tpool->has(entityOrder[index]) && tpool->has(entityOrder[index+1])) {
                                        swapIndex = tpool->getIndexById(entityOrder[index]);
                                        swapTargetIndex = tpool->getIndexById(entityOrder[index + 1]);
                                        tpool->swap(swapIndex, swapTargetIndex);
                                    }

                                    std::swap(entityOrder[index], entityOrder[index + 1]);
                                    if (targetIndex == index + 1) {
                                        break;
                                    }
                                } else if (targetIndex < index) {
                                    int swapIndex = pool->getIndexById(entityOrder[index]);
                                    int swapTargetIndex = pool->getIndexById(entityOrder[index - 1]);
                                    pool->swap(swapIndex, swapTargetIndex);

                                    auto *tpool = env->scene->getComponentPool<Transform>();
                                    if(tpool->has(entityOrder[index]) && tpool->has(entityOrder[index - 1])){
                                        swapIndex = tpool->getIndexById(entityOrder[index]);
                                        swapTargetIndex = tpool->getIndexById(entityOrder[index - 1]);
                                        tpool->swap(swapIndex, swapTargetIndex);
                                    }

                                    std::swap(entityOrder[index], entityOrder[index - 1]);
                                    if (targetIndex == index - 1) {
                                        break;
                                    }
                                } else {
                                    break;
                                }
                            } else {
                                break;
                            }

                        }

                    }

                    blockStateChange = true;
                }
            }

            if(env->input->pressed(Input::MOUSE_BUTTON_LEFT)){
                active = hovered;
            }
            if(env->input->released(Input::MOUSE_BUTTON_LEFT)){
                active = -1;
                blockStateChange = false;
            }
        }

        void clicked(EntityId id){
            bool selected = env->editor->selectionContext.isSelected(id);
            if(!shift || lastClicked == -1) {
                if (!selected) {
                    if (!control) {
                        env->editor->selectionContext.unselectAll();
                    }
                    env->editor->selectionContext.select(id);
                    lastClickedWasSelected = true;
                } else {
                    if (!control && env->editor->selectionContext.getSelected().size() > 1) {
                        env->editor->selectionContext.unselectAll();
                        env->editor->selectionContext.select(id);
                        lastClickedWasSelected = true;
                    } else {
                        env->editor->selectionContext.unselect(id);
                        lastClickedWasSelected = false;
                    }
                }
                lastClicked = id;
            }else {
                selectRange(id, lastClicked, lastClickedWasSelected);
                lastClicked = id;
            }
        }

        void selectRange(EntityId id1, EntityId id2, bool select){
            bool inRange = false;
            for (int i = 0; i < lastFrameEntityOrder.size(); i++) {
                EntityId id = lastFrameEntityOrder[i];
                bool change = false;
                if (id == id1 || id == id2) {
                    if(!inRange){
                        inRange = true;
                        change = true;
                    }
                }
                if(inRange){
                    if(select){
                        env->editor->selectionContext.select(id);
                    }else{
                        env->editor->selectionContext.unselect(id);
                    }
                }
                if (id == id1 || id == id2) {
                    if((inRange && !change) || (id == id1 && id == id2)){
                        inRange = false;
                    }
                }
            }
        }

    };
    TRI_STARTUP_CALLBACK("") {
        env->editor->addWindow(new EntitiesWindow);
    }

}

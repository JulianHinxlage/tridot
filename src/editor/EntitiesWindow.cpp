//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Editor.h"
#include "entity/Scene.h"
#include "engine/Input.h"
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
            env->scene->view<>().each([&](EntityId id){
                std::string label = "[" + std::to_string(id) + "]";
                bool selected = editor->selectionContext.isSelected(id);
                if(ImGui::Selectable(label.c_str(), &selected)){
                    if(!blockStateChange){
                        clicked(id);
                    }
                }
                if(ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax())){
                    hovered = id;
                }
                updateEntityMenu(id);
            });

            updateEntityDragging();
            updateShortcuts();
            updateMenu();

            ImGui::EndChild();
        }

        void updateHeader(){
            if(ImGui::Button("Add Entity")){
                EntityId id = editor->entityOperations.addEntity();
                editor->selectionContext.unselectAll();
                editor->selectionContext.select(id);
            }
        }

        void updateShortcuts(){
            if(control){
                if(env->input->pressed("D")){
                    std::vector<EntityId> ids;
                    for(auto &id : editor->selectionContext.getSelected()){
                        ids.push_back(editor->entityOperations.duplicateEntity(id));
                    }
                    editor->selectionContext.unselectAll();
                    for(auto &id : ids){
                        editor->selectionContext.select(id);
                    }
                }
                if(env->input->pressed("C")){
                    if(editor->selectionContext.getSelected().size() == 1) {
                        for (auto &id : editor->selectionContext.getSelected()) {
                            editor->entityOperations.copyEntity(id);
                        }
                    }else{
                        env->console->warning("cannot copy multiple entities");
                    }
                }
                if(env->input->pressed("V")){
                    EntityId copy = editor->entityOperations.pastEntity();
                    if(copy != -1){
                        editor->selectionContext.unselectAll();
                        editor->selectionContext.select(copy);
                    }
                }
            }
            if(env->input->pressed(Input::KEY_DELETE)){
                for(auto &id : editor->selectionContext.getSelected()){
                    editor->entityOperations.removeEntity(id);
                }
                editor->selectionContext.unselectAll();
            }
        }

        void updateEntityMenu(EntityId id){
            if(ImGui::BeginPopupContextItem()){
                if(ImGui::MenuItem("Delete", "Delete")){
                    if(editor->selectionContext.isSelected(id)){
                        for(auto &id2 : editor->selectionContext.getSelected()){
                            editor->entityOperations.removeEntity(id2);
                            editor->selectionContext.unselect(id2);
                        }
                    }else{
                        editor->entityOperations.removeEntity(id);
                        editor->selectionContext.unselect(id);
                    }
                }
                if(ImGui::MenuItem("Duplicate", "Ctrl+D")){
                    if(editor->selectionContext.isSelected(id)){
                        for(auto &id2 : editor->selectionContext.getSelected()){
                            std::vector<EntityId> ids;
                            for(auto &id2 : editor->selectionContext.getSelected()){
                                ids.push_back(editor->entityOperations.duplicateEntity(id2));
                            }
                            editor->selectionContext.unselectAll();
                            for(auto &id2 : ids){
                                editor->selectionContext.select(id2);
                            }
                        }
                    }else{
                        EntityId copy = editor->entityOperations.duplicateEntity(id);
                        editor->selectionContext.unselectAll();
                        editor->selectionContext.select(copy);
                    }
                }
                if(ImGui::MenuItem("Copy", "Ctrl+C")){
                    editor->entityOperations.copyEntity(id);
                }
                if(ImGui::MenuItem("Past", "Ctrl+V", false, editor->entityOperations.wasEntityCopied())){
                    EntityId copy = editor->entityOperations.pastEntity();
                    editor->selectionContext.unselectAll();
                    editor->selectionContext.select(copy);
                }
                ImGui::EndPopup();
            }
        }

        void updateMenu(){
            if (ImGui::BeginPopupContextWindow("entity", ImGuiMouseButton_Right, false)) {
                if (ImGui::MenuItem("Past", "Ctrl+V", false, editor->entityOperations.wasEntityCopied())) {
                    EntityId copy = editor->entityOperations.pastEntity();
                    editor->selectionContext.unselectAll();
                    editor->selectionContext.select(copy);
                }
                ImGui::EndPopup();
            }
        }

        void updateEntityDragging(){
            if(active != -1 && hovered != -1){
                if(hovered != active){
                    auto *pool = env->scene->getEntityPool();
                    while(true){
                        int index = pool->getIndexById(active);
                        int targetIndex = pool->getIndexById(hovered);
                        if(targetIndex > index){
                            pool->swap(index, index+1);
                            if(targetIndex == index+1){
                                break;
                            }
                        }else if(targetIndex < index){
                            pool->swap(index, index-1);
                            if(targetIndex == index-1){
                                break;
                            }
                        }else{
                            break;
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
            bool selected = editor->selectionContext.isSelected(id);
            if(!shift || lastClicked == -1) {
                if (!selected) {
                    if (!control) {
                        editor->selectionContext.unselectAll();
                    }
                    editor->selectionContext.select(id);
                    lastClickedWasSelected = true;
                } else {
                    if (!control && editor->selectionContext.getSelected().size() > 1) {
                        editor->selectionContext.unselectAll();
                        editor->selectionContext.select(id);
                        lastClickedWasSelected = true;
                    } else {
                        editor->selectionContext.unselect(id);
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
            env->scene->view<>().each([&](EntityId id) {
                bool change = false;
                if (id == id1 || id == id2) {
                    if (!inRange) {
                        inRange = true;
                        change = true;
                    }
                }
                if (inRange) {
                    if (select) {
                        editor->selectionContext.select(id);
                    } else {
                        editor->selectionContext.unselect(id);
                    }
                }
                if (id == id1 || id == id2) {
                    if (inRange && !change || (id == id1 && id == id2)) {
                        inRange = false;
                    }
                }
            });
        }

    };
    TRI_STARTUP_CALLBACK("") {
        editor->addWindow(new EntitiesWindow);
    }

}

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
        }

        void update() override {
            control = env->input->down(Input::KEY_LEFT_CONTROL) || env->input->down(Input::KEY_RIGHT_CONTROL);
            shift = env->input->down(Input::KEY_LEFT_SHIFT) || env->input->down(Input::KEY_RIGHT_SHIFT);

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
            });

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

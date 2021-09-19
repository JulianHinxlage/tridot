//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Editor.h"
#include "entity/Scene.h"
#include <imgui/imgui.h>

namespace tri {

    class PropertiesWindow : public EditorWindow {
    public:
        void startup() {
            name = "Properties";
        }

        void update() override {
            if(editor->selectionContext.getSelected().size() == 1){
                EntityId id = -1;
                for(EntityId id2 : editor->selectionContext.getSelected()){
                    id = id2;
                    break;
                }

                updateHeader(id);

                ImGui::Separator();
                ImGui::BeginChild("");

                updateEntity(id);
                updateMenu(id);

                ImGui::EndChild();
            }else if(editor->selectionContext.getSelected().size() > 1){
                //todo: multi selection properties
            }
        }

        void updateHeader(EntityId id){
            if(ImGui::Button("Add Component")){
                ImGui::OpenPopup("add");
            }
            if(ImGui::BeginPopup("add")){
                for(auto &desc : env->reflection->getDescriptors()){
                    if(desc->isComponent) {
                        if (!env->scene->hasComponent(desc->typeId, id)) {
                            if (ImGui::MenuItem(desc->name.c_str())) {
                                void *comp = editor->entityOperations.addComponent(desc->typeId, id);
                                desc->construct(comp);
                                ImGui::CloseCurrentPopup();
                            }
                        }
                    }
                }
                ImGui::EndPopup();
            }
        }

        void updateEntity(EntityId id){
            for(auto &desc : env->reflection->getDescriptors()){
                if(env->scene->hasComponent(desc->typeId, id)) {
                    if (ImGui::CollapsingHeader(desc->name.c_str())) {
                        updateComponentMenu(id, desc->typeId);
                        editor->gui.type.drawType(desc->typeId, env->scene->getComponent(desc->typeId, id));
                    }else{
                        updateComponentMenu(id, desc->typeId);
                    }
                }
            }
        }

        void updateComponentMenu(EntityId id, int typeId){
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Delete")) {
                    editor->entityOperations.removeComponent(typeId, id);
                }
                if (ImGui::MenuItem("Copy")) {
                    editor->entityOperations.copyComponent(typeId, id);
                }
                if (ImGui::MenuItem("Past", nullptr, false, editor->entityOperations.wasComponentCopied())) {
                    editor->entityOperations.pastComponent(id);
                }
                ImGui::EndPopup();
            }
        }

        void updateMenu(EntityId id){
            if (ImGui::BeginPopupContextWindow("component", ImGuiMouseButton_Right, false)) {
                if (ImGui::MenuItem("Past", nullptr, false, editor->entityOperations.wasComponentCopied())) {
                    editor->entityOperations.pastComponent(id);
                }
                ImGui::EndPopup();
            }
        }
    };

    TRI_STARTUP_CALLBACK("") {
        editor->addWindow(new PropertiesWindow);
    }

}

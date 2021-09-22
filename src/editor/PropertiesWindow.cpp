//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Editor.h"
#include "entity/Scene.h"
#include "engine/EntityInfo.h"
#include <imgui/imgui.h>

namespace tri {

    class PropertiesWindow : public EditorWindow {
    public:
        void startup() {
            name = "Properties";
        }

        void update() override {
            if(env->editor->selectionContext.getSelected().size() == 1){
                EntityId id = -1;
                for(EntityId id2 : env->editor->selectionContext.getSelected()){
                    id = id2;
                    break;
                }

                updateHeader(id);

                ImGui::Separator();
                ImGui::BeginChild("");

                updateEntity(id);
                updateMenu(id);

                ImGui::EndChild();
            }else if(env->editor->selectionContext.getSelected().size() > 1){
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
                                void *comp = env->editor->entityOperations.addComponent(desc->typeId, id);
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
            ImGui::PushID(id);
            if(env->scene->hasComponent<EntityInfo>(id)){
                EntityInfo &info = env->scene->getComponent<EntityInfo>(id);
                env->editor->gui.textInput("name", info.name);
            }
            for(auto &desc : env->reflection->getDescriptors()){
                if(env->scene->hasComponent(desc->typeId, id)) {
                    if(desc->typeId != env->reflection->getTypeId<EntityInfo>()) {
                        if (ImGui::CollapsingHeader(desc->name.c_str())) {
                            updateComponentMenu(id, desc->typeId);
                            env->editor->gui.type.drawType(desc->typeId, env->scene->getComponent(desc->typeId, id));
                        } else {
                            updateComponentMenu(id, desc->typeId);
                        }
                    }
                }
            }
            ImGui::PopID();
        }

        void updateComponentMenu(EntityId id, int typeId){
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Delete")) {
                    env->editor->entityOperations.removeComponent(typeId, id);
                }
                if (ImGui::MenuItem("Copy")) {
                    env->editor->entityOperations.copyComponent(typeId, id);
                }
                if (ImGui::MenuItem("Past", nullptr, false, env->editor->entityOperations.wasComponentCopied())) {
                    env->editor->entityOperations.pastComponent(id);
                }
                ImGui::EndPopup();
            }
        }

        void updateMenu(EntityId id){
            if (ImGui::BeginPopupContextWindow("component", ImGuiMouseButton_Right, false)) {
                if (ImGui::MenuItem("Past", nullptr, false, env->editor->entityOperations.wasComponentCopied())) {
                    env->editor->entityOperations.pastComponent(id);
                }
                ImGui::EndPopup();
            }
        }
    };

    TRI_STARTUP_CALLBACK("") {
        env->editor->addWindow(new PropertiesWindow);
    }

}

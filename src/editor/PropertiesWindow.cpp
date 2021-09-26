//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "PropertiesWindow.h"
#include "Editor.h"
#include "entity/Scene.h"
#include "engine/EntityInfo.h"
#include <imgui/imgui.h>

namespace tri {

    void PropertiesWindow::startup() {
        name = "Properties";
        lastFrameChangeTypeId = -1;
        lastFrameAnyActiveItem = false;

        noContextMenu = false;
        noWindowScroll = false;
    }

    void PropertiesWindow::update() {
        if(env->editor->selectionContext.getSelected().size() == 1){
            EntityId id = -1;
            for(EntityId id2 : env->editor->selectionContext.getSelected()){
                id = id2;
                break;
            }

            updateHeader(id);

            ImGui::Separator();
            ImGui::BeginChild("", ImVec2(0, 0), false, lastNoWindowScroll ? ImGuiWindowFlags_NoScrollWithMouse : 0);

            updateEntity(id);
            if(!lastNoContextMenu){
                updateMenu(id);
            }

            ImGui::EndChild();
        }else if(env->editor->selectionContext.getSelected().size() > 1){
            //todo: multi selection properties
        }
        lastNoWindowScroll = noWindowScroll;
        lastNoContextMenu = noContextMenu;
        noWindowScroll = false;
        noContextMenu = false;
    }

    void PropertiesWindow::updateHeader(EntityId id){
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

    void PropertiesWindow::updateEntity(EntityId id){
        ImGui::PushID(id);
        if(env->scene->hasComponent<EntityInfo>(id)){
            EntityInfo &info = env->scene->getComponent<EntityInfo>(id);
            env->editor->gui.textInput("name", info.name);
        }
        for(auto &desc : env->reflection->getDescriptors()){
            if(env->scene->hasComponent(desc->typeId, id)) {
                if(desc->typeId != env->reflection->getTypeId<EntityInfo>()) {
                    ImGui::PushID(desc->name.c_str());
                    if (ImGui::CollapsingHeader(desc->name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                        if(!lastNoContextMenu) {
                            updateComponentMenu(desc->typeId, id);
                        }
                        updateComponent(desc->typeId, id);
                    } else {
                        if(!lastNoContextMenu) {
                            updateComponentMenu(desc->typeId, id);
                        }
                    }
                    ImGui::PopID();
                }
            }
        }

        if(ImGui::IsAnyItemActive()){
            lastFrameAnyActiveItem = true;
        }else{
            lastFrameAnyActiveItem = false;
        }

        ImGui::PopID();
    }

    void PropertiesWindow::updateComponent(int typeId, EntityId id){
        auto *desc = env->reflection->getType(typeId);

        //detect changes
        void *comp = env->scene->getComponent(desc->typeId, id);
        ComponentBuffer preEditValue;
        preEditValue.set(desc->typeId, comp);

        //draw gui
        env->editor->gui.type.drawType(desc->typeId, comp);

        //detect changes
        if (env->editor->gui.type.anyTypeChange(desc->typeId, comp, preEditValue.get())) {
            if(lastFrameAnyActiveItem || ImGui::IsAnyItemActive()) {
                if (lastFrameChangeTypeId == -1) {
                    componentChangeBuffer.set(desc->typeId, preEditValue.get());
                }
                lastFrameChangeTypeId = desc->typeId;
            }
        }else{
            if(!lastFrameAnyActiveItem && !ImGui::IsAnyItemActive()) {
                if (lastFrameChangeTypeId == desc->typeId) {
                    lastFrameChangeTypeId = -1;
                    env->editor->undo.componentChanged(desc->typeId, id, componentChangeBuffer.get());
                }
            }
        }
    }

    void PropertiesWindow::updateComponentMenu(int typeId, EntityId id){
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

    void PropertiesWindow::updateMenu(EntityId id){
        if (ImGui::BeginPopupContextWindow("component", ImGuiMouseButton_Right, false)) {
            if (ImGui::MenuItem("Past", nullptr, false, env->editor->entityOperations.wasComponentCopied())) {
                env->editor->entityOperations.pastComponent(id);
            }
            ImGui::EndPopup();
        }
    }


    TRI_STARTUP_CALLBACK("") {
        env->editor->addWindow(&env->editor->properties);
    }

}

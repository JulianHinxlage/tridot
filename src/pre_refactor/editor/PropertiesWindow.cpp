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
        type = WINDOW;
        lastFrameChangeTypeId = -1;
        lastFrameAnyActiveItem = false;

        noContextMenu = false;
        noWindowScroll = false;
    }

    void PropertiesWindow::update() {
        TRI_PROFILE("Properties");
        if(env->editor->selectionContext.getSelected().size() == 1){
            EntityId id = -1;
            for(EntityId id2 : env->editor->selectionContext.getSelected()){
                id = id2;
                break;
            }

            updateHeader();

            ImGui::Separator();
            ImGui::BeginChild("", ImVec2(0, 0), false, lastNoWindowScroll ? ImGuiWindowFlags_NoScrollWithMouse : 0);

            updateEntity(id);
            if(!lastNoContextMenu){
                updateMenu();
            }

            ImGui::EndChild();
        }else if(env->editor->selectionContext.getSelected().size() > 1){

            updateHeader();

            ImGui::Separator();
            ImGui::BeginChild("", ImVec2(0, 0), false, lastNoWindowScroll ? ImGuiWindowFlags_NoScrollWithMouse : 0);

            updateMultipleEntities();
            if(!lastNoContextMenu){
                updateMenu();
            }

            ImGui::EndChild();
        }
        lastNoWindowScroll = noWindowScroll;
        lastNoContextMenu = noContextMenu;
        noWindowScroll = false;
        noContextMenu = false;
    }

    void PropertiesWindow::updateHeader(){
        if(ImGui::Button("Add Component")){
            ImGui::OpenPopup("add");
        }
        if(ImGui::BeginPopup("add")){
            for(auto &desc : env->reflection->getDescriptors()){
                if(desc && (desc->flags & Reflection::COMPONENT) && !(desc->flags & Reflection::HIDDEN_IN_EDITOR)) {

                    bool canAdd = false;
                    for(EntityId id : env->editor->selectionContext.getSelected()){
                        if (!env->scene->hasComponent(desc->typeId, id)) {
                            canAdd = true;
                            break;
                        }
                    }

                    if (canAdd) {

                        if (desc->group.empty()) {
                            if (ImGui::MenuItem(desc->name.c_str())) {
                                env->editor->undo.beginAction();
                                for (EntityId id : env->editor->selectionContext.getSelected()) {
                                    if (!env->scene->hasComponent(desc->typeId, id)) {
                                        void* comp = env->editor->entityOperations.addComponent(desc->typeId, id);
                                    }
                                }
                                env->editor->undo.endAction();
                                ImGui::CloseCurrentPopup();
                            }
                        }
                        else {
                            if (ImGui::BeginMenu(desc->group.c_str())) {
                                if (ImGui::MenuItem(desc->name.c_str())) {
                                    env->editor->undo.beginAction();
                                    for (EntityId id : env->editor->selectionContext.getSelected()) {
                                        if (!env->scene->hasComponent(desc->typeId, id)) {
                                            void* comp = env->editor->entityOperations.addComponent(desc->typeId, id);
                                        }
                                    }
                                    env->editor->undo.endAction();
                                    ImGui::CloseCurrentPopup();
                                }
                                ImGui::EndMenu();
                            }
                        }

                    }

                }
            }
            ImGui::EndPopup();
        }
    }

    void PropertiesWindow::updateMultipleEntities(){
        for(auto &desc : env->reflection->getDescriptors()) {
            if(desc && !(desc->flags & Reflection::HIDDEN_IN_EDITOR)){
                void *comp = nullptr;
                EntityId editId;
                for(EntityId id : env->editor->selectionContext.getSelected()){
                    if(env->scene->hasComponent(desc->typeId, id)){
                        comp = env->scene->getComponent(desc->typeId, id);
                        editId = id;
                        break;
                    }
                }
                if(comp != nullptr){

                    if(desc->typeId != env->reflection->getTypeId<EntityInfo>()) {
                        if(desc->typeId != env->reflection->getTypeId<Transform>()) {
                            ImGui::PushID(desc->name.c_str());
                            if (ImGui::CollapsingHeader(desc->name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                                if(!lastNoContextMenu) {
                                    updateComponentMenu(desc->typeId, editId);
                                }

                                //detect changes
                                ComponentBuffer preEditValue;
                                preEditValue.set(desc->typeId, comp);

                                //draw gui
                                env->editor->gui.typeGui.drawType(desc->typeId, comp);

                                //detect changes
                                bool anyChange = env->editor->gui.typeGui.anyTypeChange(desc->typeId, comp, preEditValue.get());
                                if (anyChange) {
                                    if(lastFrameAnyActiveItem || ImGui::IsAnyItemActive()) {
                                        if (lastFrameChangeTypeId == -1) {
                                            componentChangeBuffers.clear();
                                            for(EntityId id : env->editor->selectionContext.getSelected()) {
                                                if(env->scene->hasComponent(desc->typeId, id)){
                                                    ComponentBuffer buffer;
                                                    if(id == editId){
                                                        buffer.set(desc->typeId, preEditValue.get());
                                                    }else{
                                                        buffer.set(desc->typeId, env->scene->getComponent(desc->typeId, id));
                                                    }
                                                    componentChangeBuffers.push_back({buffer, id});
                                                }
                                            }
                                        }
                                        lastFrameChangeTypeId = desc->typeId;
                                    }
                                }

                                if(anyChange){
                                    propagateComponentChange(desc->typeId, desc->typeId, preEditValue.get(), comp);
                                }

                                //add detected changes to the undo system
                                if(!anyChange){
                                    if(!lastFrameAnyActiveItem && !ImGui::IsAnyItemActive()) {
                                        if (lastFrameChangeTypeId == desc->typeId) {
                                            lastFrameChangeTypeId = -1;
                                            env->editor->undo.beginAction();
                                            for(auto &i : componentChangeBuffers){
                                                if(env->scene->hasComponent(desc->typeId, i.second)){
                                                    env->editor->undo.componentChanged(desc->typeId, i.second, i.first.get());
                                                }
                                            }
                                            env->editor->undo.endAction();
                                            componentChangeBuffers.clear();
                                        }
                                    }
                                }

                            } else {
                                if(!lastNoContextMenu) {
                                    updateComponentMenu(desc->typeId, editId);
                                }
                            }
                            ImGui::PopID();
                        }
                    }

                }
            }
        }

        if(ImGui::IsAnyItemActive()){
            lastFrameAnyActiveItem = true;
        }else{
            lastFrameAnyActiveItem = false;
        }
    }

    void PropertiesWindow::propagateComponentChange(int rootTypeId, int typeId, void *preEdit, void *postEdit, int offset){
        auto *desc = env->reflection->getType(typeId);
        if (!desc) {
            return;
        }

        if (desc->member.size() == 0) {
            if (desc->hasEquals()) {
                if (!desc->equals(preEdit, postEdit)) {
                    for (EntityId id : env->editor->selectionContext.getSelected()) {
                        if (env->scene->hasComponent(rootTypeId, id)) {
                            void* comp = env->scene->getComponent(rootTypeId, id);
                            if (postEdit != (uint8_t*)comp + offset) {
                                desc->copy(postEdit, (uint8_t*)comp + offset);
                            }
                        }
                    }
                }
            }
        }
        for (auto& m : desc->member) {
            propagateComponentChange(rootTypeId, m.type->typeId, (uint8_t*)preEdit + m.offset, (uint8_t*)postEdit + m.offset, offset + m.offset);
        }   
    }

    void PropertiesWindow::updateEntity(EntityId id){
        ImGui::PushID(id);
        if(env->scene->hasComponent<EntityInfo>(id)){
            EntityInfo &info = env->scene->getComponent<EntityInfo>(id);
            env->editor->gui.textInput("name", info.name);
        }
        for(auto &desc : env->reflection->getDescriptors()){
            if(desc && !(desc->flags & Reflection::HIDDEN_IN_EDITOR)){
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
        if (!desc) {
            return;
        }

        //detect changes
        void *comp = env->scene->getComponent(desc->typeId, id);
        ComponentBuffer preEditValue;
        preEditValue.set(desc->typeId, comp);

        //draw gui
        env->editor->gui.typeGui.drawType(desc->typeId, comp);

        //detect changes
        if (env->editor->gui.typeGui.anyTypeChange(desc->typeId, comp, preEditValue.get())) {
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

    void PropertiesWindow::updateComponentMenu(int typeId, EntityId editId){
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Delete")) {
                env->editor->undo.beginAction();
                for(EntityId id : env->editor->selectionContext.getSelected()){
                    if (env->scene->hasComponent(typeId, id)) {
                        env->editor->entityOperations.removeComponent(typeId, id);
                    }
                }
                env->editor->undo.endAction();
            }
            if (ImGui::MenuItem("Copy")) {
                env->editor->entityOperations.copyComponent(typeId, editId);
            }
            if (ImGui::MenuItem("Past", nullptr, false, env->editor->entityOperations.wasComponentCopied())) {
                env->editor->undo.beginAction();
                for(EntityId id : env->editor->selectionContext.getSelected()){
                    env->editor->entityOperations.pastComponent(id);
                }
                env->editor->undo.endAction();
            }
            ImGui::EndPopup();
        }
    }

    void PropertiesWindow::updateMenu(){
        if (ImGui::BeginPopupContextWindow("component", ImGuiMouseButton_Right, false)) {
            if (ImGui::MenuItem("Past", nullptr, false, env->editor->entityOperations.wasComponentCopied())) {
                env->editor->undo.beginAction();
                for(EntityId id : env->editor->selectionContext.getSelected()){
                    env->editor->entityOperations.pastComponent(id);
                }
                env->editor->undo.endAction();
            }
            ImGui::EndPopup();
        }
    }


    TRI_STARTUP_CALLBACK("") {
        env->editor->addElement(&env->editor->properties);
    }

}

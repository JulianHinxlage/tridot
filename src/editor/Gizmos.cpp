//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Gizmos.h"
#include "core/core.h"
#include "render/Window.h"
#include "Editor.h"
#include "engine/Transform.h"
#include "engine/Input.h"
#include "render/RenderThread.h"
#include <imgui.h>
#include <imgui/imgui_internal.h>
#include <imguizmo/ImGuizmo.h>

namespace tri {

    void Gizmos::startup() {
        name = "Gizmos";
        type = ALWAYS_OPEN;

        env->signals->update.addCallback("Gizmos begin", [](){
            env->renderThread->addTask([]() {
                if(env->window->isOpen() && ImGui::GetCurrentContext()->WithinFrameScope){
                    ImGuizmo::BeginFrame();
                }
            });
        });
        env->signals->update.callbackOrder({"Gui begin", "Gizmos begin", "Editor"});

        operation = TRANSLATE;
        mode = LOCAL;
        pivots = CENTER;
    }

    bool isSelectedInParentChain(EntityId id){
        if(env->scene->hasComponent<Transform>(id)){
            Transform &t = env->scene->getComponent<Transform>(id);
            if(t.parent == -1){
                return false;
            }
            if(env->editor->selectionContext.isSelected(t.parent)){
                return true;
            }else{
                return isSelectedInParentChain(t.parent);
            }
        }else{
            return false;
        }
    }

    bool Gizmos::updateGizmo(const Transform &cameraTransform, const Camera &camera, const glm::vec2 &viewportPosition, const glm::vec2 &viewportSize) {
        //input control
        snappingInvert = env->input->down(Input::KEY_LEFT_CONTROL) || env->input->down(Input::KEY_RIGHT_CONTROL);
        if(env->input->pressed("R")){
            switch (operation) {
                case TRANSLATE : operation = SCALE; break;
                case SCALE : operation = ROTATE; break;
                case ROTATE : operation = TRANSLATE; break;
            }
        }
        if(env->input->pressed("F")){
            switch (mode) {
                case LOCAL : mode = WORLD; break;
                case WORLD : mode = LOCAL; break;
            }
        }
        if(env->input->pressed("T")){
            switch (pivots) {
                case CENTER : pivots = OBJECTS; break;
                case OBJECTS : pivots = CENTER; break;
            }
        }

        int entityCount = env->editor->selectionContext.getSelected().size();
        if(entityCount > 0) {
            ImGuizmo::Enable(true);
            if(camera.type == Camera::PERSPECTIVE){
                ImGuizmo::SetOrthographic(false);
            }else{
                ImGuizmo::SetOrthographic(true);
            }
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(ImGui::GetWindowPos().x + viewportPosition.x, ImGui::GetWindowPos().y + viewportPosition.y, viewportSize.x, viewportSize.y);

            //count selected
            int count = 0;
            for (auto &id : env->editor->selectionContext.getSelected()){
                if(env->scene->hasComponent<Transform>(id)){
                    count++;
                }
            }
            if(count == 0){
                return false;
            }

            //average transform of selected
            Transform avg;
            avg.scale = {0, 0, 0};
            for (auto &id : env->editor->selectionContext.getSelected()){
                if(env->scene->hasComponent<Transform>(id)){
                    Transform &t = env->scene->getComponent<Transform>(id);
                    Transform t2;
                    t2.decompose(t.getMatrix());
                    avg.position += t2.position / (float)count;
                    avg.rotation += t2.rotation / (float)count;
                    avg.scale += t2.scale / (float)count;
                }
            }

            //matrices
            glm::mat4 view = glm::inverse(cameraTransform.getMatrix());
            glm::mat4 projection = camera.projection * cameraTransform.getMatrix();
            glm::mat4 transform = avg.calculateLocalMatrix();
            glm::mat4 inverseTransform = glm::inverse(transform);

            //manipulate parameter
            ImGuizmo::OPERATION op;
            switch (operation) {
                case TRANSLATE : op = ImGuizmo::TRANSLATE; break;
                case SCALE : op = ImGuizmo::SCALE; break;
                case ROTATE : op = ImGuizmo::ROTATE; break;
            }
            ImGuizmo::MODE mo;
            switch (mode) {
                case LOCAL : mo = ImGuizmo::LOCAL; break;
                case WORLD : mo = ImGuizmo::WORLD; break;
            }
            if(operation == SCALE){
                mo = ImGuizmo::LOCAL;
            }
            float *snap = nullptr;
            if((snapping && !snappingInvert) || (!snapping && snappingInvert)){
                switch (operation) {
                    case TRANSLATE : snap = (float*)&translateSnapValues; break;
                    case SCALE : snap = (float*)&scaleSnapValues; break;
                    case ROTATE : snap = (float*)&rotateSnapValues; break;
                }
            }

            bool gizmoChange = false;
            if(ImGuizmo::Manipulate((float *) &view, (float *) &projection, op, mo, (float *) &transform, nullptr, snap)) {
                gizmoChange = true;
            }

            //detect changes
            if(ImGuizmo::IsUsing() && !lastFrameUsing){
                preModifyMatrix = avg.getMatrix();
                preModifyValues.clear();
                for (auto &id : env->editor->selectionContext.getSelected()){
                    if(env->scene->hasComponent<Transform>(id)){
                        Transform &t = env->scene->getComponent<Transform>(id);
                        preModifyValues.push_back({id, t});
                    }
                }
            }

            if(gizmoChange) {
                //apply transformation to selected
                for (auto &id : env->editor->selectionContext.getSelected()) {
                    if (env->scene->hasComponent<Transform>(id)) {
                        Transform &t = env->scene->getComponent<Transform>(id);
                        if(!isSelectedInParentChain(id)) {
                            glm::mat4 parentMatrix = t.getMatrix() * glm::inverse(t.calculateLocalMatrix());
                            if (entityCount > 1 && operation == SCALE) {
                                Transform tmp = t;
                                t.rotation = avg.rotation;
                                t.decompose(glm::inverse(parentMatrix) * transform * inverseTransform * t.getMatrix());
                                t.rotation = tmp.rotation;
                                if (pivots == OBJECTS) {
                                    t.position = tmp.position;
                                }
                            } else if (entityCount > 1 && operation == ROTATE) {
                                Transform tmp = t;
                                t.decompose(glm::inverse(parentMatrix) * transform * inverseTransform * t.getMatrix());
                                if (pivots == OBJECTS) {
                                    t.position = tmp.position;
                                }
                            } else {
                                t.decompose(glm::inverse(parentMatrix) * transform * inverseTransform * t.getMatrix());
                            }
                        }
                    }
                }
            }

            //detect changes
            if(!ImGuizmo::IsUsing() && lastFrameUsing){
                if(transform != preModifyMatrix){
                    env->editor->undo.beginAction();
                    for(auto &i : preModifyValues){
                        env->editor->undo.componentChanged(env->reflection->getTypeId<Transform>(), i.first, &i.second);
                    }
                    env->editor->undo.endAction();
                }
                preModifyMatrix = glm::mat4(1);
                preModifyValues.clear();
            }

            if(ImGuizmo::IsUsing()){
                lastFrameUsing = true;
                return true;
            }else{
                lastFrameUsing = false;
            }

        }
        return false;
    }

    void Gizmos::updateSettings() {
        ImGui::PushID("gizmos");

        bool snap = snapping ^ snappingInvert;
        ImGui::Checkbox("", &snap);
        snapping = snap ^ snappingInvert;

        ImGui::SameLine();
        switch (operation) {
            case TRANSLATE : {
                float value = translateSnapValues[0];
                ImGui::DragFloat("snap", &value, 0.01);
                translateSnapValues = glm::vec3(value, value, value);
                break;
            }
            case SCALE : {
                float value = scaleSnapValues[0];
                ImGui::DragFloat("snap", &value, 0.01);
                scaleSnapValues = glm::vec3(value, value, value);
                break;
            }
            case ROTATE : {
                float value = rotateSnapValues[0];
                ImGui::DragFloat("snap", &value, 0.5);
                rotateSnapValues = glm::vec3(value, value, value);
                break;
            }
        }

        if(ImGui::RadioButton("translate", operation == TRANSLATE)){
            operation = TRANSLATE;
        }
        ImGui::SameLine();
        if(ImGui::RadioButton("scale ", operation == SCALE)){
            operation = SCALE;
        }
        ImGui::SameLine();
        if(ImGui::RadioButton("rotate", operation == ROTATE)){
            operation = ROTATE;
        }


        if(ImGui::RadioButton("local    ", mode == LOCAL)){
            mode = LOCAL;
        }
        ImGui::SameLine();
        if(ImGui::RadioButton("world", mode == WORLD)){
            mode = WORLD;
        }

        if(ImGui::RadioButton("center   ", pivots == CENTER)){
            pivots = CENTER;
        }
        ImGui::SameLine();
        if(ImGui::RadioButton("objects", pivots == OBJECTS)){
            pivots = OBJECTS;
        }

        ImGui::PopID();
    }

    TRI_REGISTER_CALLBACK(){
        env->editor->addElement(&env->editor->gizmos);
    }

}

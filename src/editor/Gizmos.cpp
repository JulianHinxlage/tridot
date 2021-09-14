//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Gizmos.h"
#include "core/core.h"
#include "render/Window.h"
#include "Editor.h"
#include "engine/Transform.h"
#include "engine/Input.h"
#include <imgui.h>
#include <imguizmo/ImGuizmo.h>

namespace tri {

    void Gizmos::startup() {
        env->signals->update.addCallback("Gizmos.begin", [](){
            if(env->window->isOpen()){
                ImGuizmo::BeginFrame();
            }
        });
        env->signals->update.callbackOrder({"Imgui.begin", "Gizmos.begin", "Editor"});

        operation = TRANSLATE;
        mode = LOCAL;
        pivots = CENTER;
    }

    bool Gizmos::update(const Transform &cameraTransform, const Camera &camera, const glm::vec2 &viewportPosition, const glm::vec2 &viewportSize) {
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

        int entityCount = editor->selectionContext.getSelected().size();
        if(entityCount > 0) {
            ImGuizmo::Enable(true);
            if(camera.type == Camera::PERSPECTIVE){
                ImGuizmo::SetOrthographic(false);
            }else{
                ImGuizmo::SetOrthographic(true);
            }
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(ImGui::GetWindowPos().x + viewportPosition.x, ImGui::GetWindowPos().y + viewportPosition.y, viewportSize.x, viewportSize.y);

            int count = 0;
            for (auto &id : editor->selectionContext.getSelected()){
                if(env->scene->hasComponent<Transform>(id)){
                    count++;
                }
            }
            if(count == 0){
                return false;
            }

            Transform avg;
            avg.scale = {0, 0, 0};
            for (auto &id : editor->selectionContext.getSelected()){
                if(env->scene->hasComponent<Transform>(id)){
                    Transform &t = env->scene->getComponent<Transform>(id);
                    avg.position += t.position / (float)count;
                    avg.rotation += t.rotation / (float)count;
                    avg.scale += t.scale / (float)count;
                }
            }

            glm::mat4 view = glm::inverse(cameraTransform.calculateMatrix());
            glm::mat4 projection = camera.projection * cameraTransform.calculateMatrix();
            glm::mat4 transform = avg.calculateMatrix();
            glm::mat4 inverseTransform = glm::inverse(transform);

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

            if(ImGuizmo::Manipulate((float *) &view, (float *) &projection, op, mo, (float *) &transform)) {
                for (auto &id : editor->selectionContext.getSelected()) {
                    if (env->scene->hasComponent<Transform>(id)) {
                        Transform &t = env->scene->getComponent<Transform>(id);
                        if(entityCount > 1 && operation == SCALE){
                            Transform tmp = t;
                            t.rotation = avg.rotation;
                            t.decompose(transform * inverseTransform * t.calculateMatrix());
                            t.rotation = tmp.rotation;
                            if(pivots == OBJECTS){
                                t.position = tmp.position;
                            }
                        }else if(entityCount > 1 && operation == ROTATE){
                            Transform tmp = t;
                            t.decompose(transform * inverseTransform * t.calculateMatrix());
                            if(pivots == OBJECTS){
                                t.position = tmp.position;
                            }
                        }else{
                            t.decompose(transform * inverseTransform * t.calculateMatrix());
                        }
                    }
                }
            }
            if(ImGuizmo::IsUsing()){
                return true;
            }

        }
        return false;
    }

}

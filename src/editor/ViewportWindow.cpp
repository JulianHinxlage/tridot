//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "ViewportWindow.h"
#include "Editor.h"
#include "engine/Camera.h"
#include "engine/Transform.h"
#include "engine/MeshComponent.h"
#include "entity/Scene.h"
#include "render/Window.h"
#include "engine/Serializer.h"
#include "engine/Input.h"
#include "engine/EntityInfo.h"
#include "engine/AssetManager.h"
#include "entity/Prefab.h"
#include <imgui/imgui.h>

class EditorOnly{};
TRI_REGISTER_TYPE(EditorOnly);

namespace tri {

    void ViewportWindow::startup() {
        name = "Viewport";
        type = ELEMENT;
        editorCameraId = -1;
        cameraMode = EDITOR_CAMERA;

        env->signals->sceneLoad.addCallback([&](Scene *scene){
            editorCameraId = -1;
        });

        sceneBuffer = Ref<Scene>::make();
    }

    void ViewportWindow::setupFrameBuffer(Camera &cam, bool idBuffer){
        if(!cam.output) {
            cam.output = cam.output.make();
            cam.output->setAttachment({COLOR, env->window->getBackgroundColor()});
            cam.output->setAttachment({DEPTH, Color(0)});

        }
        if(idBuffer){
            if(cam.output->getAttachment((TextureAttachment) (COLOR + 1)).get() == nullptr) {
                Ref<Texture> idTexture = Ref<Texture>::make();
                idTexture->create(0, 0, TextureFormat::RGB8, false);
                cam.output->setAttachment({(TextureAttachment) (COLOR + 1), Color::white}, idTexture);
            }
        }
    }

    void ViewportWindow::setupCamera(){
        //search for camera in scene
        editorCameraId = -1;
        env->scene->view<Camera, EditorOnly>().each([&](EntityId id, Camera &cam, EditorOnly &){
            if(editorCameraId == -1){
                editorCameraId = id;
            }
        });

        if(editorCameraId == -1){
            editorCameraId = env->scene->addEntity(EditorOnly(), EntityInfo());
            Transform& t = env->scene->addComponent<Transform>(editorCameraId);
            t.position = { 0.5, 0.5, 2 };
            Camera& cam = env->scene->addComponent<Camera>(editorCameraId);
            cam.isPrimary = false;
            cam.active = true;
        }
    }

    void ViewportWindow::update(){
        if(editorCameraId == -1){
            setupCamera();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0);
        if (ImGui::Begin(name.c_str(), &isOpen)) {
            //get camera
            Ref<FrameBuffer> output = nullptr;
            ImVec2 viewportSize = ImGui::GetContentRegionAvail();
            ImVec2 viewportPosition = ImGui::GetCursorPos();
            Camera *cam = nullptr;
            Transform *camTransform = nullptr;
            env->scene->view<Camera, Transform>().each([&](EntityId id, Camera& camera, Transform &transform) {

                //select camera based on options
                if (env->editor->mode == RUNTIME || env->editor->mode == PAUSED) {
                    if(cameraMode == EDITOR_CAMERA){
                        if (id == editorCameraId) {
                            output = camera.output;
                            cam = &camera;
                            camTransform = &transform;
                        }
                    }else{
                        if (camera.isPrimary) {
                            output = camera.output;
                            cam = &camera;
                            camTransform = &transform;
                        }
                    }
                }
                else {
                    if (id == editorCameraId) {
                        output = camera.output;
                        cam = &camera;
                        camTransform = &transform;
                    }
                }

                if(camera.output) {
                    if (camera.output->getSize() != glm::vec2(viewportSize.x, viewportSize.y)) {
                        camera.output->resize(viewportSize.x, viewportSize.y);
                        camera.aspectRatio = viewportSize.x / viewportSize.y;
                    }
                }else{
                    setupFrameBuffer(camera, true);
                }
            });

            if(cam){
                cam->active = true;
            }

            if (output) {
                //draw image
                ImGui::Image((ImTextureID)(size_t)output->getAttachment(TextureAttachment::COLOR)->getId(), viewportSize, ImVec2(0, 1), ImVec2(1, 0));
                bool pickingAllowed = true;
                //gizmos
                if(cam && camTransform){
                    if(env->editor->gizmos.updateGizmo(*camTransform, *cam, {viewportPosition.x, viewportPosition.y}, {viewportSize.x, viewportSize.y})){
                        pickingAllowed = false;
                    }
                }
                //mouse picking
                if(pickingAllowed){
                    updateMousePicking(output->getAttachment((TextureAttachment)(COLOR + 1)), {viewportSize.x, viewportSize.y});
                }
            }

            //editor camera
            if(cam && camTransform) {
                if(cameraMode != FIXED_PRIMARY_CAMERA) {
                    if (ImGui::IsWindowHovered()) {
                        editorCamera.update(*cam, *camTransform);
                    }
                }
            }

            //dragging prefabs in
            std::string file = env->editor->gui.dragDropTarget(env->reflection->getTypeId<Prefab>());
            if(!file.empty()){
                Ref<Prefab> prefab = env->assets->get<Prefab>(file, true);

                glm::vec3 pos;
                if(cam && camTransform){
                    pos = camTransform->position + cam->forward;
                }

                EntityId id = prefab->createEntity(env->scene);
                env->editor->undo.entityAdded(id);
                env->editor->selectionContext.unselectAll();
                env->editor->selectionContext.select(id);

                Transform *t;
                if(env->scene->hasComponent<Transform>(id)){
                    t = &env->scene->getComponent<Transform>(id);
                }else{
                    t = &env->scene->addComponent<Transform>(id);
                }
                t->position = pos;
                t->parent = -1;
            }

        }

        ImGui::End();
        ImGui::PopStyleVar(2);
    }

    void ViewportWindow::updateMousePicking(Ref<Texture> texture, glm::vec2 viewportSize){
        if(texture){
            if(ImGui::IsItemHovered()){
                if(env->input->pressed(Input::MOUSE_BUTTON_LEFT)){

                    glm::vec2 pos = {0, 0};
                    pos.x = ImGui::GetMousePos().x - ImGui::GetItemRectMin().x;
                    pos.y = ImGui::GetMousePos().y - ImGui::GetItemRectMin().y;
                    pos /= viewportSize;
                    pos *= glm::vec2(texture->getWidth(), texture->getHeight());
                    pos.y = texture->getHeight() - pos.y;


                    Color color = texture->getPixel(pos.x, pos.y);
                    if(color.value != -1){
                        color.a = 0;
                    }
                    EntityId id = color.value;

                    bool control = env->input->down(Input::KEY_LEFT_CONTROL) || env->input->down(Input::KEY_RIGHT_CONTROL);
                    if(!control){
                        env->editor->selectionContext.unselectAll();
                    }
                    if(id != -1) {
                        if(control && env->editor->selectionContext.isSelected(id)){
                            env->editor->selectionContext.unselect(id);
                        }else{
                            env->editor->selectionContext.select(id);
                        }
                    }

                }
            }
        }
    }

    void ViewportWindow::saveEditorCameraTransform() {
        if(editorCameraId != -1){
            if(env->scene->hasComponent<Transform>(editorCameraId)){
                Transform &t = env->scene->getComponent<Transform>(editorCameraId);
                editorCameraTransformBuffer = t;
            }
        }
    }

    void ViewportWindow::restoreEditorCameraTransform() {
        if(editorCameraId != -1){
            if(env->scene->hasComponent<Transform>(editorCameraId)){
                Transform &t = env->scene->getComponent<Transform>(editorCameraId);
                t = editorCameraTransformBuffer;
            }
        }
    }

    TRI_STARTUP_CALLBACK("") {
        env->editor->addElement(&env->editor->viewport);
    }

}

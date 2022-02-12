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
#include "render/Renderer.h"
#include "render/RenderContext.h"
#include "engine/RuntimeMode.h"
#include "render/RenderThread.h"
#include "render/RenderPipeline.h"
#include "render/ShaderState.h"
#include <imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>

namespace tri {

    void ViewportWindow::startup() {
        name = "Viewport";
        type = ELEMENT;
        editorCameraId = -1;
        drawCameraId = -1;
        isHovered = false;
        viewportSize = { 0, 0 };
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

    void ViewportWindow::setupEditorCamera(){
        //search for camera in scene
        editorCameraId = -1;
        env->scene->view<Camera, EditorOnly>().each([&](EntityId id, Camera &cam, EditorOnly &){
            if(editorCameraId == -1){
                editorCameraId = id;
            }
        });

        //create editor camera
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
        TRI_PROFILE("Viewport");
        if(editorCameraId == -1){
            setupEditorCamera();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0);
        if (ImGui::Begin(name.c_str(), &isOpen)) {

            //determin camera to show
            drawCameraId = editorCameraId;
            if (env->runtime->getMode() == RuntimeMode::RUNTIME || env->runtime->getMode() == RuntimeMode::PAUSE) {
                if (cameraMode != EDITOR_CAMERA) {
                    env->scene->view<Camera>().each([&](EntityId id, Camera& camera) {
                        if (camera.isPrimary) {
                            drawCameraId = id;
                        }
                    });
                }
            }

            glm::vec2 oldSize = viewportSize;
            viewportSize = { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y };
            viewportPosition = { ImGui::GetCursorPos().x, ImGui::GetCursorPos().y };

            if (env->scene->hasComponents<Camera, Transform>(drawCameraId)) {
                Camera& camera = env->scene->getComponent<Camera>(drawCameraId);
                Transform& transform = env->scene->getComponent<Transform>(drawCameraId);
                Ref<FrameBuffer> output = camera.output;
                camera.active = true;
                if (viewportSize.y != 0) {
                    camera.aspectRatio = viewportSize.x / viewportSize.y;
                }

                if (output && output->getId() != 0) {
                    //draw rendered image to viewport
                    if (output->getAttachment(TextureAttachment::COLOR)) {
                        ImGui::Image((ImTextureID)(size_t)output->getAttachment(TextureAttachment::COLOR)->getId(), ImVec2(oldSize.x, oldSize.y), ImVec2(0, 1), ImVec2(1, 0));
                    }
                    isHovered = ImGui::IsWindowHovered();

                    //set render pipeline size and output
                    env->renderPipeline->setSize(viewportSize.x, viewportSize.y);

                    //editor camera
                    if (cameraMode != FIXED_PRIMARY_CAMERA || env->runtime->getMode() == RuntimeMode::EDIT) {
                        if (isHovered) {
                            editorCamera.update(camera, transform);
                        }
                    }

                    //gizmos
                    bool pickingAllowed = true;
                    if (env->editor->gizmos.updateGizmo(transform, camera, viewportPosition, viewportSize)) {
                        pickingAllowed = false;
                    }

                    //mouse picking
                    if (pickingAllowed) {
                        Ref<Texture> texture = output->getAttachment((TextureAttachment)(COLOR + 1));
                        if (texture) {
                            if (ImGui::IsItemHovered()) {
                                glm::vec2 pos = { 0, 0 };
                                pos.x = ImGui::GetMousePos().x - ImGui::GetItemRectMin().x;
                                pos.y = ImGui::GetMousePos().y - ImGui::GetItemRectMin().y;
                                env->renderPipeline->getPass("viewport")->addCallback("mouse picking", [this, texture, pos]() {
                                    updateMousePicking(texture, viewportSize, pos);
                                });
                            }
                        }
                    }

                    //dragging prefabs in
                    std::string file = env->editor->gui.dragDropTarget(env->reflection->getTypeId<Prefab>());
                    if (!file.empty()) {
                        Ref<Prefab> prefab = env->assets->get<Prefab>(file, true);

                        glm::vec3 pos;
                        pos = transform.position + camera.forward * 2.0f;

                        EntityId id = prefab->createEntity(env->scene);
                        env->editor->undo.entityAdded(id);
                        env->editor->selectionContext.unselectAll();
                        env->editor->selectionContext.select(id);

                        Transform& t = env->scene->getOrAddPendingComponent<Transform>(id);
                        t.position = pos;
                        t.parent = -1;
                    }

                    //draw selection overlay
                    if (env->editor->selectionContext.getSelected().size() > 0) {
                        updateSelectionOverlay(transform, camera, glm::vec2(viewportSize.x, viewportSize.y));
                        if (selectionOverlay) {
                            ImGui::SetCursorPos(ImVec2(viewportPosition.x, viewportPosition.y));
                            ImGui::Image((ImTextureID)(size_t)selectionOverlay->getAttachment(TextureAttachment::COLOR)->getId(), ImVec2(viewportSize.x, viewportSize.y), ImVec2(0, 1), ImVec2(1, 0));
                        }
                    }
                }
                else {
                    setupFrameBuffer(camera, true);
                }
            }

        }

        ImGui::End();
        ImGui::PopStyleVar(2);
    }

    void ViewportWindow::updateMousePicking(Ref<Texture> texture, glm::vec2 viewportSize, glm::vec2 pos){
        if(texture){
            if(env->input->pressed(Input::MOUSE_BUTTON_LEFT)){

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
                    if (env->scene->hasEntity(id)) {
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

    void ViewportWindow::updateSelectionOverlay(Transform &cameraTransform, Camera &camera, glm::vec2 viewportSize) {
        auto pass = env->renderPipeline->getPass("outlines");

        pass->addCallback("prepare frame buffer", [&, viewportSize]() {
            if (!selectionOverlay) {
                selectionOverlay = selectionOverlay.make();
                selectionOverlay->resize(viewportSize.x, viewportSize.y);
                selectionOverlay->setAttachment({ COLOR, Color::transparent });
            }
            else {
                if (selectionOverlay->getSize() != viewportSize) {
                    selectionOverlay->resize(viewportSize.x, viewportSize.y);
                }
            }
            selectionOverlay->clear();


            if (!selectionOverlay2) {
                selectionOverlay2 = selectionOverlay2.make();
                selectionOverlay2->resize(viewportSize.x, viewportSize.y);
                selectionOverlay2->setAttachment({ COLOR, Color::transparent });
            }
            else {
                if (selectionOverlay2->getSize() != viewportSize) {
                    selectionOverlay2->resize(viewportSize.x, viewportSize.y);
                }
            }
            selectionOverlay2->clear();
        });

        if (!selectionOverlay || !selectionOverlay2) {
            return;
        }

        pass->addCommand("blend off", BLEND_OFF);
        pass->addCommand("depth off", DEPTH_OFF);

        env->renderer->setCamera(camera.projection, cameraTransform.position, selectionOverlay2);
        env->renderer->setRenderPass(pass);


        for (auto id : env->editor->selectionContext.getSelected()) {
            if (env->scene->hasComponents<Transform, MeshComponent>(id)) {
                Transform& transform = env->scene->getComponent<Transform>(id);
                MeshComponent& mesh = env->scene->getComponent<MeshComponent>(id);

                env->renderer->submitDirect(transform.getMatrix() * glm::scale(glm::mat4(1), glm::vec3(1, 1, 1) * 1.00f), transform.position, mesh.mesh.get(), nullptr, nullptr, Color(255, 128, 0));
            }
        }
        env->renderer->setRenderPass(nullptr);


        auto call = pass->addDrawCall("outline shader");
        call->shader = env->assets->get<Shader>("shaders/outline.glsl").get();
        call->frameBuffer = selectionOverlay.get();
        call->textures.push_back(selectionOverlay2->getAttachment(COLOR).get());
        call->shaderState = Ref<ShaderState>::make();
        call->shaderState->set("uColor", Color(255, 128, 0).vec());
        call->shaderState->set("steps", 1);
    }

    void ViewportWindow::saveEditorCamera() {
        if(editorCameraId != -1){
            editorCameraBuffer.copyEntity(editorCameraId, env->scene);
        }
    }

    void ViewportWindow::restoreEditorCamera() {
        if(editorCameraId != -1){
            for (auto& comp : editorCameraBuffer.getComponents()) {
                if (env->scene->hasComponent(comp.getTypeId(), editorCameraId)) {
                    comp.get(env->scene->getComponent(comp.getTypeId(), editorCameraId));
                }
            }
        }
    }

    TRI_STARTUP_CALLBACK("") {
        env->editor->addElement(&env->editor->viewport);
    }

}

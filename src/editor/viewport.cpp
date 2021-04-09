//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/engine/Engine.h"
#include "tridot/render/Camera.h"
#include "Editor.h"
#include "EditorCamera.h"
#include <imgui.h>
#include <imguizmo/ImGuizmo.h>

using namespace tridot;

TRI_UPDATE("clear"){
    if(Editor::cameraId == -1){
        engine.view<PerspectiveCamera>().each([](ecs::EntityId id, PerspectiveCamera &camera){
            if(Editor::cameraId == -1){
                Editor::cameraId = id;
            }
        });
    }

    if(engine.has<PerspectiveCamera>(Editor::cameraId)) {
        PerspectiveCamera &camera = engine.get<PerspectiveCamera>(Editor::cameraId);
        if(camera.target.get() == nullptr) {
            camera.target = Ref<FrameBuffer>::make();
            camera.target->setTexture(COLOR);
            camera.target->setTexture(DEPTH);
            camera.target->setTexture(TextureAttachment(COLOR + 1));
        }
        if (camera.target->getSize() != Editor::viewportSize) {
            camera.target->resize(Editor::viewportSize.x, Editor::viewportSize.y);
        }
        if (camera.target->getSize().y != 0) {
            camera.aspectRatio = camera.target->getSize().x / camera.target->getSize().y;
        }
        camera.target->clear(engine.window.getBackgroundColor());

        //clear id buffer for mouse picking
        auto idBuffer = camera.target->getTexture(TextureAttachment(COLOR + 1));
        if(idBuffer){
            idBuffer->clear(Color(255, 255, 255, 255));
        }
    }

    engine.view<PerspectiveCamera>().each([](ecs::EntityId id, PerspectiveCamera &camera){
        if(id != Editor::cameraId){
            if(camera.target.get() == nullptr) {
                camera.target = Ref<FrameBuffer>::make();
                camera.target->setTexture(COLOR);
                camera.target->setTexture(DEPTH);
            }
            if (camera.target->getSize() != Editor::viewportSize) {
                camera.target->resize(Editor::viewportSize.x, Editor::viewportSize.y);
            }
            if (camera.target->getSize().y != 0) {
                camera.aspectRatio = camera.target->getSize().x / camera.target->getSize().y;
            }
            camera.target->clear(engine.window.getBackgroundColor());
        }
    });

    engine.view<OrthographicCamera>().each([](ecs::EntityId id, OrthographicCamera &camera){
        if(camera.target.get() == nullptr) {
            camera.target = Ref<FrameBuffer>::make();
            camera.target->setTexture(COLOR);
            camera.target->setTexture(DEPTH);
        }
        if (camera.target->getSize() != Editor::viewportSize) {
            camera.target->resize(Editor::viewportSize.x, Editor::viewportSize.y);
        }
        if (camera.target->getSize().y != 0) {
            camera.aspectRatio = camera.target->getSize().x / camera.target->getSize().y;
        }
        camera.target->clear(engine.window.getBackgroundColor());
    });
}


TRI_UPDATE("imguizmo begin"){
    if(engine.window.isOpen()){
        ImGuizmo::BeginFrame();
    }
}

TRI_INIT("imguizmo begin"){
    engine.onUpdate().order({"imgui begin", "imguizmo begin"});
}


TRI_UPDATE("panels"){
    if(ImGui::GetCurrentContext() != nullptr) {
        bool &open = Editor::getFlag("Viewport");
        if(open) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            if (ImGui::Begin("Viewport", &open)) {
                ImGui::SetWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
                if(engine.has<PerspectiveCamera>(Editor::cameraId)) {

                    PerspectiveCamera &camera = engine.get<PerspectiveCamera>(Editor::cameraId);
                    static EditorCamera editorCamera;
                    static ImGuizmo::OPERATION operation = ImGuizmo::OPERATION::TRANSLATE;
                    static ImGuizmo::MODE mode = ImGuizmo::MODE::LOCAL;


                    //viewport control bar
                    {
                        ImGui::BeginTable("controls", 3);
                        ImGui::TableNextColumn();

                        if(ImGui::RadioButton("translate", operation == ImGuizmo::OPERATION::TRANSLATE)){
                            operation = ImGuizmo::OPERATION::TRANSLATE;
                        }
                        ImGui::SameLine();
                        if(ImGui::RadioButton("scale", operation == ImGuizmo::OPERATION::SCALE)){
                            operation = ImGuizmo::OPERATION::SCALE;
                        }
                        ImGui::SameLine();
                        if(ImGui::RadioButton("rotate", operation == ImGuizmo::OPERATION::ROTATE)){
                            operation = ImGuizmo::OPERATION::ROTATE;
                        }

                        ImGui::TableNextColumn();

                        if(ImGui::RadioButton("local", mode == ImGuizmo::MODE::LOCAL)){
                            mode = ImGuizmo::MODE::LOCAL;
                        }
                        ImGui::SameLine();
                        if(ImGui::RadioButton("world", mode == ImGuizmo::MODE::WORLD)){
                            mode = ImGuizmo::MODE::WORLD;
                        }

                        ImGui::TableNextColumn();
                        ImGui::SliderFloat("speed", &editorCamera.speed, 0.0f, 10);
                        ImGui::EndTable();
                    }


                    if(ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
                        if(engine.input.pressed('E')){
                            if(operation == ImGuizmo::OPERATION::TRANSLATE){
                                operation = ImGuizmo::OPERATION::SCALE;
                            }else if(operation == ImGuizmo::OPERATION::SCALE){
                                operation = ImGuizmo::OPERATION::ROTATE;
                            }else if(operation == ImGuizmo::OPERATION::ROTATE){
                                operation = ImGuizmo::OPERATION::TRANSLATE;
                            }
                        }else if(engine.input.pressed('R')){
                            if(mode == ImGuizmo::MODE::LOCAL){
                                mode = ImGuizmo::MODE::WORLD;
                            }else{
                                mode = ImGuizmo::MODE::LOCAL;
                            }
                        }
                    }


                    //camera control
                    editorCamera.update(camera, ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows));
                    float mousePickX = ImGui::GetMousePos().x - (ImGui::GetWindowPos().x + ImGui::GetCursorPos().x);
                    float mousePickY = ImGui::GetMousePos().y - (ImGui::GetWindowPos().y + ImGui::GetCursorPos().y);


                    //draw to viewport panel
                    if(camera.target) {
                        Editor::viewportSize.x = ImGui::GetContentRegionAvail().x;
                        Editor::viewportSize.y = ImGui::GetContentRegionAvail().y;

                        auto texture = camera.target->getTexture(TextureAttachment(COLOR + 0));
                        ImGui::Image((void *) (size_t) texture->getId(),
                                     ImVec2(Editor::viewportSize.x, Editor::viewportSize.y),
                                     ImVec2(0, 1), ImVec2(1, 0));
                    }


                    //gizmos
                    if(Editor::selectedEntity != -1) {
                        if(engine.has<Transform>(Editor::selectedEntity)){
                            Transform &transform = engine.get<Transform>(Editor::selectedEntity);

                            ImGuizmo::SetOrthographic(false);
                            ImGuizmo::SetDrawlist();
                            ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

                            glm::mat4 matrix = transform.getMatrix();
                            glm::mat4 view = camera.getView();
                            glm::mat4 projection = camera.getPerspective();

                            bool snap = engine.input.down(Input::KEY_LEFT_CONTROL) || engine.input.down(Input::KEY_RIGHT_CONTROL);
                            glm::vec3 snapValues = glm::vec3(1) * 0.25f;
                            if(operation == ImGuizmo::OPERATION::ROTATE){
                                snapValues = glm::vec3(1) * 22.5f;
                            }

                            if(ImGuizmo::Manipulate((float *) &view, (float *) &projection, operation, mode,
                                    (float *) &matrix, nullptr, snap ? (float*)&snapValues : nullptr)){
                                ImGuizmo::DecomposeMatrixToComponents((float*)&matrix, (float*)&transform.position, (float*)&transform.rotation, (float*)&transform.scale);
                                transform.rotation = glm::radians(transform.rotation);
                            }
                        }
                    }


                    //mouse picking
                    if(camera.target) {
                        auto idBuffer = camera.target->getTexture(TextureAttachment(COLOR + 1));
                        if (idBuffer && mousePickX >= 0 && mousePickY >= 0 && mousePickX < idBuffer->getWidth() && mousePickY < idBuffer->getHeight()) {
                            int id = idBuffer->getPixel(mousePickX, idBuffer->getHeight() - mousePickY).value;

                            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
                                if (ImGui::IsMouseClicked(0) && !(ImGuizmo::IsOver() && ImGuizmo::IsUsing())) {
                                    if (id != -1 && engine.exists(id)) {
                                        Editor::selectedEntity = id;
                                    } else {
                                        Editor::selectedEntity = -1;
                                    }
                                }
                            }
                        }
                    }


                    //remove entities
                    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
                        if (engine.input.pressed(engine.input.KEY_DELETE)) {
                            if (Editor::selectedEntity != -1) {
                                engine.destroy(Editor::selectedEntity);
                                Editor::selectedEntity = -1;
                            }
                        }
                    }

                }
            }
            ImGui::End();
            ImGui::PopStyleVar();

        }
    }
}

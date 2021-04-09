//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/engine/Engine.h"
#include "tridot/render/Camera.h"
#include "Editor.h"
#include "EditorCamera.h"
#include <imgui.h>

using namespace tridot;

TRI_UPDATE("clear"){
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
                    editorCamera.update(camera, ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows));

                    if(camera.target) {
                        //mouse picking
                        auto idBuffer = camera.target->getTexture(TextureAttachment(COLOR + 1));
                        if(idBuffer) {
                            float x = ImGui::GetMousePos().x - (ImGui::GetWindowPos().x + ImGui::GetCursorPos().x);
                            float y = ImGui::GetMousePos().y - (ImGui::GetWindowPos().y + ImGui::GetCursorPos().y);
                            int id = idBuffer->getPixel(x, idBuffer->getHeight() - y).value;

                            if(ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)){
                                if(ImGui::IsMouseClicked(0)){
                                    if(id != -1 && engine.exists(id)){
                                        Editor::selectedEntity = id;
                                    }else{
                                        Editor::selectedEntity = -1;
                                    }
                                }
                            }
                        }

                        //draw to viewport panel
                        Editor::viewportSize.x = ImGui::GetContentRegionAvail().x;
                        Editor::viewportSize.y = ImGui::GetContentRegionAvail().y;

                        auto texture = camera.target->getTexture(TextureAttachment(COLOR + 0));
                        ImGui::Image((void *) (size_t) texture->getId(),
                                     ImVec2(Editor::viewportSize.x, Editor::viewportSize.y),
                                     ImVec2(0, 1), ImVec2(1, 0));
                    }
                }
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }
    }
}

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/engine/Engine.h"
#include "tridot/render/Camera.h"
#include "Editor.h"
#include <imgui.h>

using namespace tridot;

TRI_UPDATE("clear"){
    if(engine.has<PerspectiveCamera>(Editor::cameraId)) {
        PerspectiveCamera &camera = engine.get<PerspectiveCamera>(Editor::cameraId);
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
                    Editor::viewportSize.x = ImGui::GetContentRegionAvail().x;
                    Editor::viewportSize.y = ImGui::GetContentRegionAvail().y;
                    if(camera.target) {
                        auto texture = camera.target->getTexture(COLOR);
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

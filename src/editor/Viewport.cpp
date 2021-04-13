//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Viewport.h"
#include "Editor.h"
#include "tridot/engine/Engine.h"
#include <glm/gtc/matrix_transform.hpp>

namespace tridot {

    void Viewport::init() {
        engine.onUpdate().add("imguizmo begin", [](){
            if(engine.window.isOpen()){
                ImGuizmo::BeginFrame();
            }
        });
        engine.onUpdate().add("clear", [](){
            Editor::viewport.clear();
        });
        engine.onUpdate().order({"imgui begin", "imguizmo begin"});
    }

    void Viewport::update() {
        if(ImGui::GetCurrentContext() != nullptr) {
            bool &open = Editor::getFlag("Viewport");
            if (open) {
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
                if (ImGui::Begin("Viewport", &open)) {
                    ImGui::SetWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);

                    updateControlBar();
                    draw();
                    updateGizmos();
                    updateMousePicking();

                    //camera control
                    if(engine.has<PerspectiveCamera>(Editor::cameraId)) {
                        PerspectiveCamera &camera = engine.get<PerspectiveCamera>(Editor::cameraId);
                        editorCamera.update(camera, ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows), !controlDown);
                    }
                }
                ImGui::End();
                ImGui::PopStyleVar();
            }
        }
    }

    void Viewport::updateControlBar() {
        controlDown = engine.input.down(Input::KEY_LEFT_CONTROL) || engine.input.down(Input::KEY_RIGHT_CONTROL);

        ImGui::BeginTable("controls", 5, ImGuiTableFlags_SizingStretchSame);
        ImGui::TableSetupColumn("1", ImGuiTableColumnFlags_None, 0.30);
        ImGui::TableSetupColumn("2", ImGuiTableColumnFlags_None, 0.15);
        ImGui::TableSetupColumn("3", ImGuiTableColumnFlags_None, 0.05);
        ImGui::TableSetupColumn("4", ImGuiTableColumnFlags_None, 0.25);
        ImGui::TableSetupColumn("5", ImGuiTableColumnFlags_None, 0.25);

        //toggle runtime mode
        ImGui::TableNextColumn();
        if(ImGui::Checkbox("play", &Editor::runtime)){
            if(Editor::runtime){
                Editor::enableRuntime();
            }else{
                Editor::disableRuntime();
            }
        }

        //radio buttons for operation
        ImGui::SameLine();
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

        //radio buttons for mode
        ImGui::TableNextColumn();
        if(ImGui::RadioButton("local", mode == ImGuizmo::MODE::LOCAL)){
            mode = ImGuizmo::MODE::LOCAL;
        }
        ImGui::SameLine();
        if(ImGui::RadioButton("world", mode == ImGuizmo::MODE::WORLD)){
            mode = ImGuizmo::MODE::WORLD;
        }

        //snap
        ImGui::TableNextColumn();
        bool snapTmp = snap ^ controlDown;
        if(ImGui::Checkbox("snap", &snapTmp)){
            snap = snapTmp ^ controlDown;
        }

        //snap values
        ImGui::TableNextColumn();
        if(operation == ImGuizmo::OPERATION::TRANSLATE){
            ImGui::DragFloat("snap value", &snapValueTranslate, 0.01);
        }if(operation == ImGuizmo::OPERATION::SCALE){
            ImGui::DragFloat("snap value", &snapValueScale, 0.01);
        }if(operation == ImGuizmo::OPERATION::ROTATE){
            ImGui::DragFloat("snap value", &snapValueRotate, 1.0);
        }

        ImGui::TableNextColumn();
        ImGui::SliderFloat("speed", &editorCamera.speed, 0.0f, 10);

        //cycle gizmo mode and operation
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
            if(controlDown){
                if(engine.input.pressed('D')){
                    Editor::selection.duplicateAll();
                }
            }
            if (engine.input.pressed(engine.input.KEY_DELETE)) {
                Editor::selection.destroyAll();
            }
        }
        ImGui::EndTable();
    }

    void Viewport::draw() {
        if(engine.has<PerspectiveCamera>(Editor::cameraId)) {
            PerspectiveCamera &camera = engine.get<PerspectiveCamera>(Editor::cameraId);
            if (camera.target) {
                viewportSize.x = ImGui::GetContentRegionAvail().x;
                viewportSize.y = ImGui::GetContentRegionAvail().y;

                auto texture = camera.target->getTexture(TextureAttachment(COLOR + 0));
                ImGui::Image((void *) (size_t) texture->getId(),
                             ImVec2(viewportSize.x, viewportSize.y),
                             ImVec2(0, 1), ImVec2(1, 0));

                mousePickPosition.x = ImGui::GetMousePos().x - ImGui::GetItemRectMin().x;
                mousePickPosition.y = ImGui::GetMousePos().y - ImGui::GetItemRectMin().y;
            }
        }
    }

    void Viewport::updateGizmos() {
        if(engine.has<PerspectiveCamera>(Editor::cameraId)) {
            PerspectiveCamera &camera = engine.get<PerspectiveCamera>(Editor::cameraId);

            ecs::EntityId selectedEntity = Editor::selection.getSingleSelection();
            if(selectedEntity != -1) {

                //single entity
                if(engine.has<Transform>(selectedEntity)){
                    Transform &transform = engine.get<Transform>(selectedEntity);

                    ImGuizmo::SetOrthographic(false);
                    ImGuizmo::SetDrawlist();
                    ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

                    glm::mat4 matrix = transform.getMatrix();
                    glm::mat4 view = camera.getView();
                    glm::mat4 projection = camera.getPerspective();

                    glm::vec3 snapValues = glm::vec3(1);
                    if(operation == ImGuizmo::OPERATION::TRANSLATE){
                        snapValues *= snapValueTranslate;
                    }if(operation == ImGuizmo::OPERATION::SCALE){
                        snapValues *= snapValueScale;
                    }if(operation == ImGuizmo::OPERATION::ROTATE){
                        snapValues *= snapValueRotate;
                    }

                    if(ImGuizmo::Manipulate((float *) &view, (float *) &projection, operation,
                                            operation == ImGuizmo::OPERATION::SCALE ? ImGuizmo::MODE::LOCAL : mode,
                                            (float *) &matrix, nullptr, (snap ^ controlDown) ? (float*)&snapValues : nullptr)){
                        ImGuizmo::DecomposeMatrixToComponents((float*)&matrix, (float*)&transform.position, (float*)&transform.rotation, (float*)&transform.scale);
                        transform.rotation = glm::radians(transform.rotation);
                    }
                }

            }else{

                //multiple entities
                if(Editor::selection.selectedEntities.size() > 1) {

                    Transform transform;
                    int count = 0;
                    for (auto &sel : Editor::selection.selectedEntities) {
                        ecs::EntityId id = sel.first;
                        if (engine.has<Transform>(id)) {
                            Transform &t = engine.get<Transform>(id);
                            transform.position += t.position;
                            transform.scale *= t.scale;
                            transform.rotation += t.rotation;
                            count++;
                        }
                    }
                    transform.position /= count;
                    transform.rotation /= count;
                    transform.scale = glm::pow(transform.scale, glm::vec3(1, 1, 1) / (float)count);


                    ImGuizmo::SetOrthographic(false);
                    ImGuizmo::SetDrawlist();
                    ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

                    glm::mat4 matrix = transform.getMatrix();
                    glm::mat4 inverse = glm::inverse(matrix);
                    glm::mat4 view = camera.getView();
                    glm::mat4 projection = camera.getPerspective();

                    glm::vec3 snapValues = glm::vec3(1);
                    if(operation == ImGuizmo::OPERATION::TRANSLATE){
                        snapValues *= snapValueTranslate;
                    }if(operation == ImGuizmo::OPERATION::SCALE){
                        snapValues *= snapValueScale;
                    }if(operation == ImGuizmo::OPERATION::ROTATE){
                        snapValues *= snapValueRotate;
                    }

                    if(ImGuizmo::Manipulate((float *) &view, (float *) &projection, operation,
                            operation == ImGuizmo::OPERATION::SCALE ? ImGuizmo::MODE::LOCAL : mode,
                            (float *) &matrix, nullptr, (snap ^ controlDown) ? (float*)&snapValues : nullptr)){

                        for (auto &sel : Editor::selection.selectedEntities) {
                            ecs::EntityId id = sel.first;
                            if (engine.has<Transform>(id)) {
                                Transform &t = engine.get<Transform>(id);

                                glm::mat4 m = matrix * inverse * t.getMatrix();
                                ImGuizmo::DecomposeMatrixToComponents((float*)&m, (float*)&t.position, (float*)&t.rotation, (float*)&t.scale);
                                t.rotation = glm::radians(t.rotation);
                            }
                        }
                    }

                }

            }
        }
    }

    void Viewport::updateMousePicking() {
        if(engine.has<PerspectiveCamera>(Editor::cameraId)) {
            PerspectiveCamera &camera = engine.get<PerspectiveCamera>(Editor::cameraId);
            if(camera.target) {
                auto idBuffer = camera.target->getTexture(TextureAttachment(COLOR + 1));
                if (idBuffer && mousePickPosition.x >= 0 && mousePickPosition.y >= 0 &&
                        mousePickPosition.x < idBuffer->getWidth() && mousePickPosition.y < idBuffer->getHeight()) {
                    int id = idBuffer->getPixel(mousePickPosition.x, idBuffer->getHeight() - mousePickPosition.y).value;

                    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
                        if (ImGui::IsMouseClicked(0) && !(ImGuizmo::IsOver() && ImGuizmo::IsUsing())) {
                            if (id != -1 && engine.exists(id)) {
                                Editor::selection.select(id, !(engine.input.down(Input::KEY_RIGHT_CONTROL) || engine.input.down(Input::KEY_LEFT_CONTROL)));
                            } else {
                                Editor::selection.unselect();
                            }
                        }
                    }
                }
            }
        }
    }

    float updateFramebuffer(Ref<FrameBuffer> &target, bool useIdBuffer, glm::vec2 viewportSize){
        if(target.get() == nullptr) {
            target = Ref<FrameBuffer>::make();
            target->setTexture(COLOR);
            target->setTexture(DEPTH);
            if(useIdBuffer){
                target->setTexture(TextureAttachment(COLOR + 1));
            }
        }
        if (target->getSize() != viewportSize) {
            target->resize(viewportSize.x, viewportSize.y);
        }
        float aspectRatio = 1;
        if (target->getSize().y != 0) {
            aspectRatio = target->getSize().x / target->getSize().y;
        }
        target->clear(engine.window.getBackgroundColor());

        if(useIdBuffer) {
            auto idBuffer = target->getTexture(TextureAttachment(COLOR + 1));
            if (idBuffer) {
                idBuffer->clear(Color(255, 255, 255, 255));
            }
        }
        return aspectRatio;
    }

    void Viewport::clear() {
        engine.view<PerspectiveCamera>().each([&](ecs::EntityId id, PerspectiveCamera &camera){
            if(Editor::cameraId == -1){
                Editor::cameraId = id;
            }
            if(id == Editor::cameraId) {
                updateFramebuffer(camera.target, true, viewportSize);
            }else {
                updateFramebuffer(camera.target, false, viewportSize);
            }
        });

        engine.view<OrthographicCamera>().each([&](ecs::EntityId id, OrthographicCamera &camera){
            updateFramebuffer(camera.target, false, viewportSize);
        });
    }

}

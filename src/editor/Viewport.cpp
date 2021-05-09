//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Viewport.h"
#include "Editor.h"
#include "tridot/engine/Engine.h"
#include "tridot/components/RenderComponent.h"
#include <GL/glew.h>

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
        engine.onUpdate().order({"window", "clear", "rendering"});
    }

    void Viewport::update() {
        if(ImGui::GetCurrentContext() != nullptr) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            EditorGui::window("Viewport", [this]() {
                updateControlBar();
                drawSelectionOverlay();
                draw();
                updateGizmos();
                updateMousePicking();

                //camera control
                if (engine.has<PerspectiveCamera>(Editor::cameraId)) {
                    PerspectiveCamera &camera = engine.get<PerspectiveCamera>(Editor::cameraId);
                    editorCamera.update(camera, ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows), !controlDown);
                }
            });
            ImGui::PopStyleVar();
        }
    }

    void Viewport::updateControlBar() {
        controlDown = engine.input.down(Input::KEY_LEFT_CONTROL) || engine.input.down(Input::KEY_RIGHT_CONTROL);

        ImGui::BeginTable("controls", 6, ImGuiTableFlags_SizingStretchSame);
        ImGui::TableSetupColumn("1", ImGuiTableColumnFlags_None, 0.25);
        ImGui::TableSetupColumn("2", ImGuiTableColumnFlags_None, 0.125);
        ImGui::TableSetupColumn("3", ImGuiTableColumnFlags_None, 0.075);
        ImGui::TableSetupColumn("4", ImGuiTableColumnFlags_None, 0.05);
        ImGui::TableSetupColumn("5", ImGuiTableColumnFlags_None, 0.25);
        ImGui::TableSetupColumn("6", ImGuiTableColumnFlags_None, 0.25);

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

        //outlines
        ImGui::TableNextColumn();
        ImGui::Checkbox("outlines", &selectionOverlay);

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

                viewportPosition.x = ImGui::GetCursorPos().x;
                viewportPosition.y = ImGui::GetCursorPos().y;
                auto texture = camera.target->getTexture(TextureAttachment(COLOR + 0));
                if(texture) {
                    ImGui::Image((void *) (size_t) texture->getId(),
                                 ImVec2(viewportSize.x, viewportSize.y),
                                 ImVec2(0, 1), ImVec2(1, 0));
                }
                mousePickPosition.x = ImGui::GetMousePos().x - ImGui::GetItemRectMin().x;
                mousePickPosition.y = ImGui::GetMousePos().y - ImGui::GetItemRectMin().y;

                if(selectionOverlayTarget && selectionOverlay){
                    ImGui::SetCursorPos(ImVec2(viewportPosition.x, viewportPosition.y));
                    auto overlay = selectionOverlayTarget->getTexture(TextureAttachment(COLOR + 0));
                    if(overlay){
                        ImGui::Image((void *) (size_t) overlay->getId(),
                                     ImVec2(viewportSize.x, viewportSize.y),
                                     ImVec2(0, 1), ImVec2(1, 0));
                    }
                }
            }
        }
    }

    bool isParentSelected(EntityId id){
        if(engine.has<Transform>(id)){
            Transform &t = engine.get<Transform>(id);
            if(t.parent.id != -1){
                for (auto &sel : Editor::selection.entities) {
                    if(sel.first == t.parent.id){
                        return true;
                    }
                }
                return isParentSelected(t.parent.id);
            }
        }
        return false;
    }

    void Viewport::updateGizmos() {
        if(engine.has<PerspectiveCamera>(Editor::cameraId)) {
            PerspectiveCamera &camera = engine.get<PerspectiveCamera>(Editor::cameraId);

            EntityId selectedEntity = Editor::selection.getSingleSelection();
            if(selectedEntity != -1) {

                //single entity
                if(engine.has<Transform>(selectedEntity)){
                    Transform &transform = engine.get<Transform>(selectedEntity);

                    ImGuizmo::Enable(true);
                    ImGuizmo::SetOrthographic(false);
                    ImGuizmo::SetDrawlist();
                    ImGuizmo::SetRect(ImGui::GetWindowPos().x + viewportPosition.x, ImGui::GetWindowPos().y + viewportPosition.y, viewportSize.x, viewportSize.y);

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

                    static bool modifiedLast = false;
                    if(ImGuizmo::Manipulate((float *) &view, (float *) &projection, operation,
                                            operation == ImGuizmo::OPERATION::SCALE ? ImGuizmo::MODE::LOCAL : mode,
                                            (float *) &matrix, nullptr, (snap ^ controlDown) ? (float*)&snapValues : nullptr)){
                        if (!modifiedLast) {
                            Editor::undo.beginAction();
                            Editor::undo.changeComponent(selectedEntity, Reflection::get<Transform>(), &transform);
                        }

                        matrix = glm::inverse(transform.parent.matrix) * matrix;
                        transform.decompose(matrix);
                        modifiedLast = true;

                        Editor::undo.changeComponent(selectedEntity, Reflection::get<Transform>(), &transform);
                    } else {
                        if (!ImGuizmo::IsUsing()) {
                            if (modifiedLast) {
                                Editor::undo.endAction();
                            }
                            modifiedLast = false;
                        }
                    }
                }

            }else{

                //multiple entities
                if(Editor::selection.entities.size() > 1) {

                    Transform transform;
                    int count = 0;
                    for (auto& sel : Editor::selection.entities) {
                        EntityId id = sel.first;
                        if (engine.has<Transform>(id)) {
                            count++;
                        }
                    }

                    bool differeingRotation = false;
                    int index = 0;
                    for (auto &sel : Editor::selection.entities) {
                        EntityId id = sel.first;
                        if (engine.has<Transform>(id)) {
                            Transform t;
                            t.decompose(engine.get<Transform>(id).getMatrix());
                            transform.position += t.position / (float)count;
                            transform.scale *= glm::pow(t.scale, glm::vec3(1, 1, 1) / (float)count);
                            transform.rotation += t.rotation / (float)count;
                            index++;
                            if (t.rotation / (float)count != transform.rotation / (float)index) {
                                differeingRotation = true;
                            }
                        }
                    }

                    ImGuizmo::Enable(true);
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

                    static bool modifiedLast = false;
                    if(ImGuizmo::Manipulate((float *) &view, (float *) &projection, operation,
                            operation == ImGuizmo::OPERATION::SCALE ? ImGuizmo::MODE::LOCAL : mode,
                            (float *) &matrix, nullptr, (snap ^ controlDown) ? (float*)&snapValues : nullptr)){

                        if (!modifiedLast) {
                            Editor::undo.beginAction();
                        }

                        for (auto &sel : Editor::selection.entities) {
                            EntityId id = sel.first;
                            if (engine.has<Transform>(id)) {
                                Transform &t = engine.get<Transform>(id);


                                if(!isParentSelected(id)){
                                    if (!modifiedLast) {
                                        Editor::undo.changeComponent(id, Reflection::get<Transform>(), &t);
                                    }

                                    glm::mat4 m = matrix * inverse * t.getMatrix();
                                    m = glm::inverse(t.parent.matrix) * m;

                                    if (differeingRotation && operation == ImGuizmo::OPERATION::SCALE) {
                                        glm::vec3 rotation = t.rotation;
                                        t.decompose(m);
                                        t.rotation = rotation;
                                    } else {
                                        t.decompose(m);
                                    }

                                    Editor::undo.changeComponent(id, Reflection::get<Transform>(), &t);
                                }
                            }
                        }

                        modifiedLast = true;
                    }
                    else {
                        if (!ImGuizmo::IsUsing()) {
                            if (modifiedLast) {
                                Editor::undo.endAction();
                            }
                            modifiedLast = false;
                        }
                    }

                }else{
                    ImGuizmo::Enable(false);
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

                    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
                        if (ImGui::IsMouseClicked(0) && !(ImGuizmo::IsOver() && ImGuizmo::IsUsing())) {
                            int id = idBuffer->getPixel(mousePickPosition.x, idBuffer->getHeight() - mousePickPosition.y).value;
                            if (id != -1 && engine.exists(id)) {
                                if(Editor::selection.isSelected(id) && controlDown){
                                    Editor::selection.unselect(id);
                                }else{
                                    Editor::selection.select(id, !controlDown);
                                }
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
        engine.view<PerspectiveCamera>().each([&](EntityId id, PerspectiveCamera &camera){
            if(Editor::cameraId == -1){
                Editor::cameraId = id;
            }
            if(id == Editor::cameraId) {
                camera.aspectRatio = updateFramebuffer(camera.target, true, viewportSize);
            }else {
                camera.aspectRatio = updateFramebuffer(camera.target, false, viewportSize);
            }
        });

        engine.view<OrthographicCamera>().each([&](EntityId id, OrthographicCamera &camera){
            camera.aspectRatio = updateFramebuffer(camera.target, false, viewportSize);
        });
    }

    void Viewport::drawSelectionOverlay() {
        if(engine.has<PerspectiveCamera>(Editor::cameraId) && selectionOverlay) {
            PerspectiveCamera &camera = engine.get<PerspectiveCamera>(Editor::cameraId);

            updateFramebuffer(selectionOverlayTarget, false, viewportSize);
            selectionOverlayTarget->clear(Color::transparent);
            engine.pbRenderer.begin(camera.getProjection(), camera.position, selectionOverlayTarget);

            for (auto &sel : Editor::selection.entities) {
                EntityId id = sel.first;
                if (engine.exists(id)) {
                    if (engine.has<Transform>(id)) {
                        Transform &transform = engine.get<Transform>(id);
                        if (engine.has<RenderComponent>(id)) {
                            RenderComponent &rc = engine.get<RenderComponent>(id);
                            transform.scale *= 1.05f;
                            engine.pbRenderer.submit(transform.getMatrix(), Color(255,128,0,255), rc.mesh.get(), nullptr, id);
                            transform.scale /= 1.05f;
                        }
                    }
                }
            }

            engine.pbRenderer.end();
            selectionOverlayTarget->bind();
            glClear(GL_DEPTH_BUFFER_BIT);

            for (auto &sel : Editor::selection.entities) {
                EntityId id = sel.first;
                if (engine.exists(id)) {
                    if (engine.has<Transform>(id)) {
                        Transform &transform = engine.get<Transform>(id);
                        if (engine.has<RenderComponent>(id)) {
                            RenderComponent &rc = engine.get<RenderComponent>(id);
                            engine.pbRenderer.submit(transform.getMatrix(), Color::transparent, rc.mesh.get(), nullptr, id);
                        }
                    }
                }
            }

            engine.pbRenderer.end();
            glFinish();
        }
    }

}

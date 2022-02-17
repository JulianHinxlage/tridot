//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Editor.h"
#include "render/Window.h"
#include "core/util/StrUtil.h"
#include "engine/Serializer.h"
#include "engine/Transform.h"
#include "engine/MeshComponent.h"
#include "engine/Input.h"
#include "engine/AssetManager.h"
#include "engine/EntityInfo.h"
#include "engine/RuntimeMode.h"
#include "render/RenderPipeline.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace tri {

    TRI_REGISTER_SYSTEM_INSTANCE(Editor, env->editor);

    void Editor::startup(){
        updated = false;

        env->signals->update.callbackOrder({"Gui begin", "Camera", "MeshComponent", "Editor"});
        env->signals->postStartup.addCallback([&](){
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

            //set colors of gui
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 0.3));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.4));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.5));
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1, 1, 1, 0.2));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1, 1, 1, 0.3));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1, 1, 1, 0.4));
            ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(1, 1, 1, 0.1));
            ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(1, 1, 1, 0.25));
            ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(1, 1, 1, 0.40));
            ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(1, 1, 1, 0.1));
            ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImVec4(1, 1, 1, 0.25));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1, 1, 1, 0.35));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1, 1, 1, 0.45));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(1, 1, 1, 0.55));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1, 1, 1, 0.35));
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1, 1, 1, 0.35));
            ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1, 1, 1, 0.45));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.116, 0.125, 0.133, 1));
            ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.177, 0.177, 0.177, 1));
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.238, 0.238, 0.238, 1));
            ImGui::PushStyleColor(ImGuiCol_DragDropTarget, ImVec4(0.0, 0.32, 1.0, 1));

            std::string fontFile = env->assets->searchFile("");
            if (!fontFile.empty()) {
                ImGui::GetIO().Fonts->AddFontFromFileTTF(fontFile.c_str(), 16);
            }

            //set editor layout/config file
            const char *configFile = "editor.ini";
            if (!std::filesystem::exists(configFile)) {
                std::vector<std::string> configFileList = { "../editor.ini" , "../../editor.ini" };
                for (auto& file : configFileList) {
                    if (!file.empty()) {
                        if (std::filesystem::exists(file)) {
                            std::filesystem::copy(file, configFile);
                            break;
                        }
                    }
                }
            }
            ImGui::GetIO().IniFilename = configFile;

            setupFlagSaving();
            setupSettingsSaving();
        });
        env->signals->postUpdate.addCallback([this](){
            updated = false;
        });


        env->signals->postStartup.addCallback("Editor", [this]() {
            for(auto &element : elements){
                if(element){
                    element->startup();
                }
            }
        });
        env->signals->postStartup.callbackOrder({ "Editor", "RuntimeMode" });

        sceneBuffer = Ref<Scene>::make();

        env->signals->preShutdown.addCallback("Editor", [&](){
            env->runtime->setMode(RuntimeMode::SHUTDOWN);
            if (std::filesystem::exists("autosave2.scene")) {
                std::filesystem::copy("autosave2.scene", "autosave3.scene", std::filesystem::copy_options::overwrite_existing);
            }
            if(std::filesystem::exists("autosave.scene")){
                std::filesystem::copy("autosave.scene", "autosave2.scene", std::filesystem::copy_options::overwrite_existing);
            }
            env->scene->save("autosave.scene");
        });

        env->signals->runtimeModeChanged.addCallback("Editor", [&]() {
            auto mode = env->runtime->getMode();
            auto previous = env->runtime->getPreviousMode();

            if (mode == RuntimeMode::EDIT || (mode == RuntimeMode::SHUTDOWN && previous != RuntimeMode::EDIT)) {
                if (viewport.cameraMode == EDITOR_CAMERA) {
                    viewport.saveEditorCamera();
                }
                env->signals->sceneEnd.invoke(env->scene);
                {
                    TRI_PROFILE("copyScene");
                    env->scene->copy(*sceneBuffer);
                    sceneBuffer->clear();
                }
                if (viewport.cameraMode == EDITOR_CAMERA) {
                    viewport.restoreEditorCamera();
                }
                env->signals->sceneLoad.invoke(env->scene);
            }
            else if (mode == RuntimeMode::RUNTIME) {
                if (previous == RuntimeMode::EDIT) {
                    {
                        TRI_PROFILE("copyScene");
                        sceneBuffer->clear();
                        sceneBuffer->copy(*env->scene);
                    }
                }
                env->signals->sceneBegin.invoke(env->scene);
            }
            else if (mode == RuntimeMode::PAUSE) {
                if (previous == RuntimeMode::EDIT) {
                    {
                        TRI_PROFILE("copyScene");
                        sceneBuffer->clear();
                        sceneBuffer->copy(*env->scene);
                    }
                }
                env->signals->sceneEnd.invoke(env->scene);
            }
        });
    }

    void Editor::update() {
        env->renderPipeline->getPass("editor")->addCallback("editor", [&]() {
        if (!updated && ImGui::GetCurrentContext() && ImGui::GetCurrentContext()->WithinFrameScope) {
            updated = true;
            ImGui::DockSpaceOverViewport();
            updateMenuBar();

            //windows
            for (auto &element : elements) {
                if (element) {
                    if(element->isOpen || element->type == EditorElement::ALWAYS_OPEN){
                        if (element->type == EditorElement::WINDOW || element->type == EditorElement::DEBUG_WINDOW) {
                            if (ImGui::Begin(element->name.c_str(), &element->isOpen)) {
                                element->update();
                            }
                            ImGui::End();
                        }
                        else {
                            element->update();
                        }
                    }
                }
            }

            //undo/redo
            bool control = env->input->down(Input::KEY_LEFT_CONTROL) || env->input->down(Input::KEY_RIGHT_CONTROL);
            if(control && env->input->pressed("Y")){
                bool shift = env->input->down(Input::KEY_LEFT_SHIFT) || env->input->down(Input::KEY_RIGHT_SHIFT);
                if(shift){
                    TRI_PROFILE("redo");
                    undo.redo();
                }else{
                    TRI_PROFILE("undo");
                    undo.undo();
                }
            }

            //runtime
            if (env->input->pressed(Input::KEY_F6)) {
                if (env->runtime->getMode() == RuntimeMode::PAUSE) {
                    env->runtime->setMode(RuntimeMode::RUNTIME);
                }
                else {
                    env->runtime->setMode(RuntimeMode::PAUSE);
                }
            }
            if(env->input->pressed(Input::KEY_F5)){
                if(env->runtime->getMode() == RuntimeMode::RUNTIME || env->runtime->getMode() == RuntimeMode::PAUSE){
                    env->runtime->setMode(RuntimeMode::EDIT);
                }else if(env->runtime->getMode() == RuntimeMode::EDIT){
                    env->runtime->setMode(RuntimeMode::RUNTIME);
                }
            }

        }
        });
    }

    void Editor::shutdown(){
        for (auto &element : elements) {
            if(element){
                element->shutdown();
            }
        }
    }

    void Editor::addElement(EditorElement* element){
        elements.push_back(std::shared_ptr<EditorElement>(element, [](EditorElement *ptr){}));
    }

    void Editor::updateMenuBar() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open", "Ctrl+O")) {
                    env->editor->gui.fileGui.openBrowseWindow("Open", "Open Scene", env->reflection->getTypeId<Scene>(), [](const std::string &file){
                        Scene::loadMainScene(file);
                    });
                }
                if (ImGui::MenuItem("Save", "Ctrl+S")) {
                    env->assets->unload(env->scene->file);
                    Ref<Scene> buffer(true);
                    buffer->copy(*env->scene);
                    env->threads->addThread([buffer](){
                        buffer->save(buffer->file);
                    });
                }
                if (ImGui::MenuItem("Save As", "Ctrl+Shift+S")) {
                    env->editor->gui.fileGui.openBrowseWindow("Save", "Save Scene As", env->reflection->getTypeId<Scene>(), [](const std::string &file){
                        env->assets->unload(file);
                        env->scene->file = file;
                        Ref<Scene> buffer(true);
                        buffer->copy(*env->scene);
                        env->threads->addThread([buffer](){
                            buffer->save(buffer->file);
                        });
                    });
                }
                if (ImGui::MenuItem("Close")) {
                    env->scene->clear();
                }
                if (ImGui::MenuItem("Exit")) {
                    env->window->close();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                for (auto &element : elements) {
                    if (element) {
                        if (element->type == EditorElement::WINDOW || element->type == EditorElement::ELEMENT) {
                            ImGui::MenuItem(element->name.c_str(), nullptr, &element->isOpen);
                        }
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Debug")) {
                for (auto &element : elements) {
                    if (element) {
                        if (element->type == EditorElement::DEBUG_WINDOW) {
                            ImGui::MenuItem(element->name.c_str(), nullptr, &element->isOpen);
                        }
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        bool control = env->input->down(Input::KEY_LEFT_CONTROL) || env->input->down(Input::KEY_RIGHT_CONTROL);
        bool shift = env->input->down(Input::KEY_LEFT_SHIFT) || env->input->down(Input::KEY_RIGHT_SHIFT);
        if (control && env->input->pressed("S")) {
            if(shift){
                env->editor->gui.fileGui.openBrowseWindow("Save", "Save Scene", env->reflection->getTypeId<Scene>(), [](const std::string &file){
                    env->assets->unload(file);
                    env->scene->file = file;
                    Ref<Scene> buffer(true);
                    buffer->copy(*env->scene);
                    env->threads->addThread([buffer](){
                        buffer->save(buffer->file);
                    });
                });
            }else{
                env->assets->unload(env->scene->file);
                Ref<Scene> buffer(true);
                buffer->copy(*env->scene);
                env->threads->addThread([buffer](){
                    buffer->save(buffer->file);
                });
            }
        }
        if (control && env->input->pressed("O")) {
            env->editor->gui.fileGui.openBrowseWindow("Open", "Open Scene", env->reflection->getTypeId<Scene>(), [](const std::string &file){
                Scene::loadMainScene(file);
            });
        }
    }

    void Editor::setupFlagSaving(){
        ImGuiSettingsHandler handler;
        handler.TypeName = "OpenFlags";
        handler.TypeHash = ImHashStr("OpenFlags");
        handler.ReadOpenFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name) -> void* {
            if (std::string(name) == "") {
                return (void*)1;
            }
            else {
                return nullptr;
            }
        };
        handler.ReadLineFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line) {
            auto parts = StrUtil::split(line, "=");
            if (parts.size() >= 2) {
                for (auto& element : env->editor->elements) {
                    if (element && element->type != EditorElement::ALWAYS_OPEN) {
                        if (element->name == parts[0]) {
                            try {
                                element->isOpen = std::stoi(parts[1]);
                            }
                            catch (...) {}
                        }
                    }
                }
            }
        };
        handler.WriteAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
            buf->append("[OpenFlags][]\n");
            for (auto& element : env->editor->elements) {
                if (element && element->type != EditorElement::ALWAYS_OPEN) {
                    buf->appendf("%s=%i\n", element->name.c_str(), (int)element->isOpen);
                }
            }
            buf->appendf("\n");
        };
        ImGui::GetCurrentContext()->SettingsHandlers.push_back(handler);
    }

    void Editor::setupSettingsSaving(){
        ImGuiSettingsHandler handler;
        handler.TypeName = "ViewportSettings";
        handler.TypeHash = ImHashStr("ViewportSettings");
        handler.ReadOpenFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name) -> void* {
            if (std::string(name) == "") {
                return (void*)1;
            }
            else {
                return nullptr;
            }
        };
        handler.ReadLineFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line) {
            auto parts = StrUtil::split(line, "=");
            if (parts.size() >= 2) {

                if (parts[0] == "speed") {
                    try {
                        env->editor->viewport.editorCamera.speed = std::stof(parts[1]);
                    }
                    catch (...) {}
                }
                else if (parts[0] == "cameraMode") {
                    try {
                        env->editor->viewport.cameraMode = (tri::EditorCameraMode)std::stoi(parts[1]);
                    }
                    catch (...) {}
                }
                else if (parts[0] == "gizmoOperation") {
                    try {
                        env->editor->gizmos.operation = (tri::Gizmos::Operation)std::stoi(parts[1]);
                    }
                    catch (...) {}
                }
                else if (parts[0] == "gizmoMode") {
                    try {
                        env->editor->gizmos.mode = (tri::Gizmos::Mode)std::stoi(parts[1]);
                    }
                    catch (...) {}
                }
                else if (parts[0] == "gizmoPivots") {
                    try {
                        env->editor->gizmos.pivots = (tri::Gizmos::Pivots)std::stoi(parts[1]);
                    }
                    catch (...) {}
                }
                else if (parts[0] == "snap") {
                    try {
                        env->editor->gizmos.snapping = std::stoi(parts[1]);
                    }
                    catch (...) {}
                }
                else if (parts[0] == "snapTranslate") {
                    try {
                        env->editor->gizmos.translateSnapValues.x = std::stof(parts[1]);
                        env->editor->gizmos.translateSnapValues.y = std::stof(parts[1]);
                        env->editor->gizmos.translateSnapValues.z = std::stof(parts[1]);
                    }
                    catch (...) {}
                }
                else if (parts[0] == "snapScale") {
                    try {
                        env->editor->gizmos.scaleSnapValues.x = std::stof(parts[1]);
                        env->editor->gizmos.scaleSnapValues.y = std::stof(parts[1]);
                        env->editor->gizmos.scaleSnapValues.z = std::stof(parts[1]);
                    }
                    catch (...) {}
                }
                else if (parts[0] == "snapRotate") {
                    try {
                        env->editor->gizmos.rotateSnapValues.x = std::stof(parts[1]);
                        env->editor->gizmos.rotateSnapValues.y = std::stof(parts[1]);
                        env->editor->gizmos.rotateSnapValues.z = std::stof(parts[1]);
                    }
                    catch (...) {}
                }



                for (auto& element : env->editor->elements) {
                    if (element && element->type != EditorElement::ALWAYS_OPEN) {
                        if (element->name == parts[0]) {
                            try {
                                element->isOpen = std::stoi(parts[1]);
                            }
                            catch (...) {}
                        }
                    }
                }
            }
        };
        handler.WriteAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
            buf->append("[ViewportSettings][]\n");
            buf->appendf("%s=%f\n", "speed", env->editor->viewport.editorCamera.speed);
            buf->appendf("%s=%i\n", "cameraMode", (int)env->editor->viewport.cameraMode);
            buf->appendf("%s=%i\n", "snap", env->editor->gizmos.snapping);
            buf->appendf("%s=%f\n", "snapTranslate", env->editor->gizmos.translateSnapValues.x);
            buf->appendf("%s=%f\n", "snapScale", env->editor->gizmos.scaleSnapValues.x);
            buf->appendf("%s=%f\n", "snapRotate", env->editor->gizmos.rotateSnapValues.x);
            buf->appendf("%s=%i\n", "gizmoOperation", (int)env->editor->gizmos.operation);
            buf->appendf("%s=%i\n", "gizmoMode", (int)env->editor->gizmos.mode);
            buf->appendf("%s=%i\n", "gizmoPivots", (int)env->editor->gizmos.pivots);
            buf->appendf("\n");
        };
        ImGui::GetCurrentContext()->SettingsHandlers.push_back(handler);
    }

    class ImguiDemo : public EditorElement {
    public:
        void startup() {
            name = "ImGui Demo";
            type = DEBUG_WINDOW;
        }
        void update() override{
            ImGui::ShowDemoWindow(&isOpen);
        }
    };
    TRI_STARTUP_CALLBACK("") {
        //env->editor->addElement<ImguiDemo>();
    }

}
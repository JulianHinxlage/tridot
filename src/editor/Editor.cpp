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
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace tri {

    TRI_REGISTER_SYSTEM_INSTANCE(Editor, env->editor);

    void Editor::startup(){
        updated = false;
        mode = EDIT;

        env->signals->update.callbackOrder({"Imgui/begin", "Editor", "MeshComponent"});
        env->signals->postStartup.addCallback([](){
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

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

            ImGui::GetIO().IniFilename = "editor.ini";

            ImGuiSettingsHandler handler;
            handler.TypeName = "UserData";
            handler.TypeHash = ImHashStr("UserData");
            handler.ReadOpenFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name) -> void* {
                if (std::string(name) == "OpenFlags") {
                    return (void*)1;
                }
                else {
                    return nullptr;
                }
            };
            handler.ReadLineFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line) {
                auto parts = StrUtil::split(line, "=");
                if (parts.size() >= 2) {
                    for (auto &element : env->editor->elements) {
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
                buf->append("[UserData][OpenFlags]\n");
                for (auto &element : env->editor->elements) {
                    if (element && element->type != EditorElement::ALWAYS_OPEN) {
                        buf->appendf("%s=%i\n", element->name.c_str(), (int)element->isOpen);
                    }
                }
            };
            ImGui::GetCurrentContext()->SettingsHandlers.push_back(handler);
        });
        env->signals->postUpdate.addCallback([this](){
            updated = false;
        });


        env->signals->postStartup.addCallback([this](){
            for(auto &element : elements){
                if(element){
                    element->profileName = "Editor/" + element->name;
                    element->startup();
                }
            }
        });

        sceneBuffer = Ref<Scene>::make();

        env->signals->preShutdown.addCallback("Editor", [&](){
            if(std::filesystem::exists("autosave.scene")){
                std::filesystem::copy("autosave.scene", "autosave2.scene", std::filesystem::copy_options::overwrite_existing);
            }
            env->scene->save("autosave.scene");
        });
    }

    void Editor::update() {
        if (!updated && ImGui::GetCurrentContext() && ImGui::GetCurrentContext()->WithinFrameScope) {
            updated = true;
            ImGui::DockSpaceOverViewport();
            updateMenuBar();

            //windows
            for (auto &element : elements) {
                if (element) {
                    if(element->isOpen || element->type == EditorElement::ALWAYS_OPEN){
                        TRI_PROFILE(element->profileName.c_str());
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
                    undo.redo();
                }else{
                    undo.undo();
                }
            }

            //runtime
            if (env->input->pressed(Input::KEY_F6)) {
                if (mode == RUNTIME) {
                    setMode(PAUSED);
                } else if (mode == PAUSED) {
                    setMode(RUNTIME);
                }
            }
            if(env->input->pressed(Input::KEY_F5)){
                if(mode == RUNTIME || mode == PAUSED){
                    setMode(EDIT);
                }else if(mode == EDIT){
                    setMode(RUNTIME);
                }
            }

            gui.update();
        }
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
                if(ImGui::MenuItem("Reset Scene")){
                    env->signals->sceneLoad.invoke(env->scene);
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

    void Editor::setMode(RuntimeMode mode) {
        if(this->mode != mode) {
            if (mode == EDIT) {
                if(viewport.cameraMode == EDITOR_CAMERA){
                    viewport.saveEditorCameraTransform();
                }
                env->scene->copy(*sceneBuffer);
                sceneBuffer->clear();
                if(viewport.cameraMode == EDITOR_CAMERA) {
                    viewport.restoreEditorCameraTransform();
                }
                env->signals->sceneLoad.invoke(env->scene);
                this->mode = EDIT;
            } else if (mode == RUNTIME) {
                if(this->mode == EDIT){
                    sceneBuffer->clear();
                    sceneBuffer->copy(*env->scene);
                }
                this->mode = RUNTIME;
            } else if (mode == PAUSED) {
                if(this->mode == EDIT){
                    sceneBuffer->clear();
                    sceneBuffer->copy(*env->scene);
                }
                this->mode = PAUSED;
            }
        }
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
        env->editor->addElement<ImguiDemo>();
    }

}
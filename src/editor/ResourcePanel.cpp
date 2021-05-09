//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "ResourcePanel.h"
#include "tridot/engine/Engine.h"
#include "tridot/engine/Plugin.h"
#include "Editor.h"
#include "EditorGui.h"
#include <imgui.h>

using namespace tridot;

namespace tridot {

    template<typename T>
    void updateResourcePanel(const char* name, const char* panelName) {
        static Ref<T> res = nullptr;
        EditorGui::window(panelName, [name](){
            EditorGui::drawResourceSelection(res, name);

            std::string newName = std::string("new ") + std::string(name);

            if (ImGui::Button("new")) {
                ImGui::OpenPopup(newName.c_str());
            }

            ImGui::SameLine();
            if (ImGui::Button("open")) {
                ImGui::OpenPopup("Open File");
            }

            if (res) {
                ImGui::SameLine();
                if (ImGui::Button("remove")) {
                    engine.resources.remove(engine.resources.getName(res));
                    res = nullptr;
                }
            }

            bool popupOpen = true;
            if (ImGui::BeginPopupModal(newName.c_str(), &popupOpen)) {
                if (!popupOpen) {
                    ImGui::CloseCurrentPopup();
                }
                static char buffer[256];
                ImGui::InputText("Name", buffer, sizeof(buffer));
                if (ImGui::Button("Create")) {
                    res = engine.resources.get<T>(std::string(buffer), ResourceManager::JUST_CREATE);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            if (ImGui::BeginPopupModal("Open File", &popupOpen)) {
                if (!popupOpen) {
                    ImGui::CloseCurrentPopup();
                }
                static char buffer[256];
                ImGui::InputText("File", buffer, sizeof(buffer));
                if (ImGui::Button("Open")) {
                    res = engine.resources.get<T>(std::string(buffer));
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::Separator();
            ImGui::BeginChild("resource");
            if (res.get() != nullptr) {
                EditorGui::drawType(*res.get());
            }
            ImGui::EndChild();
        });
    }

    void ResourcePanel::update() {
        updateResourcePanel<Material>("Material", "Materials");
        updateResourcePanel<Texture>("Texture", "Textures");
        updateResourcePanel<Mesh>("Mesh", "Meshes");
        updateResourcePanel<Shader>("Shader", "Shaders");
        updateResourcePanel<Plugin>("Plugin", "Plugins");
    }

}
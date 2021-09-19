//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "EditorGui.h"
#include "Editor.h"
#include "engine/AssetManager.h"
#include <imgui.h>

namespace tri {

    void EditorGui::startup() {
        file.startup();
    }

    void EditorGui::dragDropSource(int typeId, const std::string &file) {
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload(env->reflection->getType(typeId)->name.c_str(), file.data(), file.size());
            ImGui::Text("%s", file.c_str());
            ImGui::EndDragDropSource();
        }
    }

    std::string EditorGui::dragDropTarget(int typeId) {
        if (ImGui::BeginDragDropTarget()) {
            if (ImGui::AcceptDragDropPayload(env->reflection->getType(typeId)->name.c_str())) {
                auto *payload = ImGui::GetDragDropPayload();
                ImGui::EndDragDropTarget();
                return std::string((char *) payload->Data, payload->DataSize);
            }
            ImGui::EndDragDropTarget();
        }
        return "";
    }

    void EditorGui::update() {
        file.update();
    }

}

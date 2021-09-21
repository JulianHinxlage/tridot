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

    void EditorGui::textInput(const std::string &label, std::string &text, const std::string &hint) {
        if(inputBuffer.size() == 0){
            inputBuffer.resize(1024);
        }
        strcpy(inputBuffer.data(), text.c_str());
        ImGui::InputTextWithHint(label.c_str(), hint.c_str(), inputBuffer.data(), inputBuffer.capacity() - 1);
        text = inputBuffer.c_str();
    }

    void EditorGui::update() {
        file.update();
    }

}

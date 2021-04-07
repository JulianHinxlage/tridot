//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/engine/Engine.h"
#include "Editor.h"
#include <imgui.h>

using namespace tridot;
using namespace ecs;

TRI_UPDATE("panels"){
    if(ImGui::GetCurrentContext() != nullptr) {
        bool &open = editor.getFlag("Entities");
        if(open) {
            if (ImGui::Begin("Entities", &open)) {
                engine.view<>().each([&](EntityId id) {
                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow |
                            ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                    if (id == editor.selectedEntity) {
                        flags |= ImGuiTreeNodeFlags_Selected;
                    }
                    ImGui::TreeNodeEx((void *) (size_t) id, flags, "Entity %i", id);
                    if (ImGui::IsItemClicked()) {
                        if(editor.selectedEntity == id){
                            editor.selectedEntity = -1;
                        }else{
                            editor.selectedEntity = id;
                        }
                    }
                });
                if (ImGui::Button("add Entity")) {
                    editor.selectedEntity = engine.create();
                }
            }
            ImGui::End();
        }
    }
}

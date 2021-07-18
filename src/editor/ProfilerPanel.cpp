//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "ProfilerPanel.h"
#include "tridot/util/StrUtil.h"
#include "EditorGui.h"
#include "tridot/engine/Engine.h"

namespace tridot {

    void ProfilerPanel::init() {

    }

    void updateNode(Profiler::Node &node, bool defaultOpen = false){
        ImGuiTreeNodeFlags flags = 0;
        if(defaultOpen){
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }
        if(node.nodes.size() == 0){
            flags |= ImGuiTreeNodeFlags_Leaf;
        }
        if(ImGui::TreeNodeEx(node.name.c_str(), flags, "%.2f ms (x%i) %s", node.time * 1000.0f, (int)node.count, node.name.c_str())){
            for(auto &n : node.nodes){
                updateNode(n);
            }
            ImGui::TreePop();
        }
    }

    void ProfilerPanel::update() {
        EditorGui::window("Profiler", [](){
            updateNode(engine.profiler.node, true);
        });
    }

}

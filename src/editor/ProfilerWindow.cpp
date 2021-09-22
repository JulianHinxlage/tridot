//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Editor.h"
#include "engine/Time.h"
#include <imgui/imgui.h>

namespace tri {

    class ProfilerWindow : public EditorWindow {
    public:
        void startup() {
            name = "Profiler";
            isDebugWindow = true;
        }

        void update() override {
            if (env->time->frameTicks(0.5)) {
                env->profiler->updateNodes();
            }
            updateNode(env->profiler->node, ImGuiTreeNodeFlags_DefaultOpen);
        }

        void updateNode(Profiler::Node &node, ImGuiTreeNodeFlags flags = 0) {
            if (node.nodes.size() == 0) {
                flags |= ImGuiTreeNodeFlags_Leaf;
            }
            if (ImGui::TreeNodeEx(node.name.c_str(), flags, "%.2f ms (x %i) %s", node.time * 1000.0f, (int)node.count, node.name.c_str())) {
                for (auto& n : node.nodes) {
                    updateNode(n);
                }
                ImGui::TreePop();
            }
        }

    };
    TRI_STARTUP_CALLBACK("") {
        env->editor->addWindow(new ProfilerWindow);
    }

}

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Editor.h"
#include "engine/Time.h"
#include <imgui/imgui.h>

namespace tri {

    class ProfilerWindow : public EditorElement {
    public:
        void startup() {
            name = "Profiler";
            type = DEBUG_WINDOW;
        }
        int previousDisplayFrame = 0;
        int displayFrame = 0;
        bool allFrames = false;

        void update() override {
            if (env->time->frameTicks(0.5)) {
                previousDisplayFrame = displayFrame;
                displayFrame = env->profiler->getCurrentFrame() - 1;
            }
            for(auto &node : env->profiler->getPhaseNodes()){
                if(node->name == "update"){
                    allFrames = false;
                }else{
                    allFrames = true;
                }
                updateNode(*node);
            }
        }

        void updateNode(Profiler::Node &node, ImGuiTreeNodeFlags flags = 0) {
            if (node.children.size() == 0) {
                flags |= ImGuiTreeNodeFlags_Leaf;
            }
            float count = 0;
            float sum = 0;

            if(allFrames){
                for(auto &i : node.frames){
                    for (auto time : i.second) {
                        sum += time;
                        count++;
                    }
                }
            }else {
                int frames = 0;
                for (int i = previousDisplayFrame + 1; i <= displayFrame; i++) {
                    if (node.frames.find(i) != node.frames.end()) {
                        auto &times = node.frames[i];
                        for (auto time : times) {
                            sum += time;
                            count++;
                        }
                        frames++;
                    }
                }
                if(frames > 0){
                    sum /= frames;
                    count /= frames;
                }
            }

            if (ImGui::TreeNodeEx(node.name.c_str(), flags, "%.2f ms (x %i) %s", sum * 1000.0f, (int)count, node.name.c_str())) {
                for (auto& child : node.children) {
                    updateNode(*child);
                }
                ImGui::TreePop();
            }

        }

    };

#if TRI_ENABLE_OWN_PROFILER
    TRI_STARTUP_CALLBACK("") {
        env->editor->addElement<ProfilerWindow>();
    }
#endif

}

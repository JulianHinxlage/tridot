//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Editor.h"
#include "render/RenderPipeline.h"
#include <imgui/imgui.h>

namespace tri {

    class RenderPipelineWindow : public EditorElement {
    public:

        void startup() override {
            name = "Render Pipeline";
            type = DEBUG_WINDOW;
        }

        void update() override {
            TRI_PROFILE("Render Pipeline Window");
            for (auto& pass : env->pipeline->getRenderPasses()) {
                if (pass) {
                    ImGui::PushID(pass->name.c_str());
                    if (ImGui::Checkbox("", &pass->active)) {}
                    ImGui::PopID();
                    ImGui::SameLine();

                    if (ImGui::TreeNodeEx(pass->name.c_str())) {
                        for (int i = 0; i < pass->steps.size(); i++) {
                            auto &step = pass->steps[i];
                            std::string str = step.name;
                            if (str.empty()) {
                                str = std::string("step ") + std::to_string(i);
                            }

                            ImGui::PushID(str.c_str());
                            if (ImGui::Checkbox("", &step.active)) {}
                            ImGui::PopID();
                            ImGui::SameLine();

                            if (ImGui::TreeNodeEx(str.c_str())) {
                                env->editor->gui.typeGui.drawType(env->reflection->getTypeId<RenderPassStep>(), &step, false);

                                if (step.textures.size() > 0) {
                                    if (ImGui::TreeNodeEx("textures")) {
                                        for (int i = 0; i < step.textures.size(); i++) {
                                            std::string str = std::string("texture ") + std::to_string(i);
                                            if (ImGui::TreeNodeEx(str.c_str())) {
                                                auto& texture = step.textures[i];
                                                float aspect = 0;
                                                if (texture->getHeight() != 0) {
                                                    aspect = (float)texture->getWidth() / (float)texture->getHeight();
                                                }
                                                ImGui::Image((void*)(size_t)texture->getId(), ImVec2(200 * aspect, 200), ImVec2(0, 1), ImVec2(1, 0));
                                                ImGui::TreePop();
                                            }
                                        }
                                        ImGui::TreePop();
                                    }
                                }

                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                }
                else {
                    if (ImGui::TreeNodeEx("<null>", ImGuiTreeNodeFlags_Leaf)) {
                        ImGui::TreePop();
                    }
                }
            }
        }


    };

    TRI_STARTUP_CALLBACK("") {
        env->editor->addElement<RenderPipelineWindow>();
    }

}

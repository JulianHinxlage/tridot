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
            
            auto& passes = env->renderPipeline->getRootPass()->subPasses;
            for (int i = 0; i < passes.size(); i++) {
                updatePass(passes[i].get(), i);
            }
        }

        void updatePass(RenderPass *pass, int index) {
            std::string name = pass->name;
            if (name.empty()) {
                name = std::string("step ") + std::to_string(index);
            }

            ImGui::PushID(name.c_str());
            if (ImGui::Checkbox("", &pass->active)) {}
            ImGui::PopID();
            ImGui::SameLine();

            if (ImGui::TreeNodeEx(name.c_str())) {
                for (int i = 0; i < pass->subPasses.size(); i++) {
                    auto subPass = pass->subPasses[i];
                    updatePass(subPass.get(), i);
                }

                if (pass->type != RenderPass::NODE) {
                    env->editor->gui.typeGui.drawConstants(env->reflection->getTypeId<RenderPass::Type>(), &pass->type, "type");
                }

                if (pass->type == RenderPass::DRAW_CALL) {
                    RenderPassDrawCall* call = (RenderPassDrawCall*)pass;

                    Ref<Mesh> mesh(call->mesh);
                    env->editor->gui.typeGui.drawMember(env->reflection->getTypeId<Ref<Mesh>>(), &mesh, "mesh", nullptr, nullptr, false);

                    Ref<Shader> shader(call->shader);
                    env->editor->gui.typeGui.drawMember(env->reflection->getTypeId<Ref<Shader>>(), &shader, "shader", nullptr, nullptr, false);

                    env->editor->gui.typeGui.drawMember(env->reflection->getTypeId<int>(), &call->instanceCount, "instanceCount", nullptr, nullptr, false);

                    Ref<FrameBuffer> frameBuffer(call->frameBuffer);
                    env->editor->gui.typeGui.drawMember(env->reflection->getTypeId<Ref<FrameBuffer>>(), &frameBuffer, "frameBuffer", nullptr, nullptr, false);

                    env->editor->gui.typeGui.drawMember(env->reflection->getTypeId<Ref<ShaderState>>(), &call->shaderState, "shaderState", nullptr, nullptr, false);
                }
                else if (pass->type == RenderPass::DRAW_COMMAND) {
                    env->editor->gui.typeGui.drawType(env->reflection->getTypeId<RenderPassDrawCommand>(), pass, false);
                }

                ImGui::TreePop();
            }

        }
    };

    TRI_STARTUP_CALLBACK("") {
        env->editor->addElement<RenderPipelineWindow>();
    }

}

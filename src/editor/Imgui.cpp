//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "core/core.h"
#include "render/RenderContext.h"
#include "render/Window.h"
#include "render/FrameBuffer.h"
#include "engine/Input.h"
#include "render/RenderPipeline.h"
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <GL/glew.h>
#include <glfw/glfw3.h>

namespace tri {

    class Imgui : public System {
    public:
        bool inFrame;

        void startup() override {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
#if TRI_WINDOWS
            //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif
            ImGui::StyleColorsDark();
            ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)RenderContext::get(), true);
            ImGui_ImplOpenGL3_Init("#version 330");
            inFrame = false;


            env->signals->update.addCallback("Gui begin", [this]() {
                begin();
            });

            env->signals->update.addCallback("Gui end", [this]() {
                end();
            });

            env->signals->update.callbackOrder({ "Gui end", "Window", "Gui begin" });
        }

        void begin() {
            env->renderPipeline->getPass("gui begin")->addCallback("gui begin", [&]() {
                if (!inFrame && env->window->isOpen()) {
                    ImGuiIO& io = ImGui::GetIO();
                    io.MouseWheel += (float)env->input->getMouseWheelDelta();

                    ImGui_ImplOpenGL3_NewFrame();
                    ImGui_ImplGlfw_NewFrame();
                    ImGui::NewFrame();
                    inFrame = true;
                }
            });
        }

        void end() {
            env->renderPipeline->getPass("gui end")->addCallback("gui end", [&]() {
                env->input->allowInputs = !ImGui::GetIO().WantTextInput;
                if (inFrame && env->window->isOpen()) {
                    FrameBuffer::unbind();
                    ImGui::Render();

                    auto* data = ImGui::GetDrawData();
                    if (data) {
                        ImGui_ImplOpenGL3_RenderDrawData(data);
                    }
                    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                        GLFWwindow* context = glfwGetCurrentContext();
                        ImGui::RenderPlatformWindowsDefault();
                        ImGui::UpdatePlatformWindows();
                        glfwMakeContextCurrent(context);
                    }
                    inFrame = false;
                }
            });
        }

        void shutdown() override {
            env->signals->update.removeCallback("Gui begin");
            env->signals->update.removeCallback("Gui end");

            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
        }
    };

    TRI_REGISTER_SYSTEM(Imgui);

}
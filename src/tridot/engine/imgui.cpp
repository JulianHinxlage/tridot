//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/core/Environment.h"
#include "tridot/engine/Profiler.h"
#include "tridot/render/RenderContext.h"
#include "tridot/render/Window.h"
#include "tridot/render/FrameBuffer.h"
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace tridot;

bool inFrame = false;

void imguiInit(){
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)RenderContext::get(), true);
    ImGui_ImplOpenGL3_Init("#version 130");
    env->events->update.callbackOrder({"imgui end", "window", "imgui begin"});
}

TRI_UPDATE_CALLBACK("imgui begin"){
    TRI_PROFILE("imgui/begin")
    if(env->window->isOpen()){
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        inFrame = true;
    }else{
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}

TRI_UPDATE_CALLBACK("imgui end"){
    TRI_PROFILE("imgui/end")
    if(inFrame && env->window->isOpen()){
        FrameBuffer::unbind();
        ImGui::Render();
        auto *data = ImGui::GetDrawData();
        if(data){
            ImGui_ImplOpenGL3_RenderDrawData(data);
        }
        inFrame = false;
    }
}

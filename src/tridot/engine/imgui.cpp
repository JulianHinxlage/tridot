//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Engine.h"
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

using namespace tridot;

bool inFrame = false;

TRI_INIT("imgui"){
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)engine.window.getContext(), true);
    ImGui_ImplOpenGL3_Init("#version 130");
    engine.onUpdate().order({"imgui end", "window", "imgui begin"});
}

TRI_UPDATE("imgui begin"){
    if(engine.window.isOpen()){
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

TRI_UPDATE("imgui end"){
    if(inFrame && engine.window.isOpen()){
        FrameBuffer::unbind();
        ImGui::Render();
        auto *data = ImGui::GetDrawData();
        if(data){
            ImGui_ImplOpenGL3_RenderDrawData(data);
        }
        inFrame = false;
    }
}

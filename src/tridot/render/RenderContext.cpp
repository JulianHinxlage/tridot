//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "RenderContext.h"
#include "tridot/core/Log.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace tridot {

    void *RenderContext::context = nullptr;

    void *RenderContext::create() {
        //init glfw
        static bool glfwInited = false;
        if(!glfwInited){
            if(glfwInit() != GLFW_TRUE) {
                Log::error("failed to initialize GLFW");
                return nullptr;
            }
            glfwInited = true;
        }

        //create context
        GLFWwindow *context = glfwCreateWindow(1, 1, "", nullptr, (GLFWwindow*)RenderContext::context);
        if(!context){
            Log::error("failed to create GLFW context");
            return nullptr;
        }
        set(context);

        //init glew
        static bool glewInited = false;
        if(!glewInited){
            if(glewInit() != GLEW_OK) {
                Log::error("failed to initialize GLEW");
                return nullptr;
            }
            glewInited = true;
        }

        return context;
    }

    void *RenderContext::get() {
        return context;
    }

    void RenderContext::set(void *context) {
        if(context != RenderContext::context){
            glfwMakeContextCurrent((GLFWwindow*)context);
            RenderContext::context = context;
        }
    }

    void RenderContext::destroy() {
        if(context != nullptr){
            glfwDestroyWindow((GLFWwindow*)context);
            set(nullptr);
        }
    }

    void RenderContext::setDepth(bool enabled) {
        if(enabled){
            glEnable(GL_DEPTH_TEST);
        }else{
            glDisable(GL_DEPTH_TEST);
        }
    }

    void RenderContext::setBlend(bool enabled) {
        if(enabled){
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }else{
            glDisable(GL_BLEND);
        }
    }

    void RenderContext::setCull(bool enabled, bool front) {
        if(enabled){
            glEnable(GL_CULL_FACE);
            glCullFace(front ? GL_FRONT : GL_BACK);
        }else{
            glDisable(GL_CULL_FACE);
        }
    }

    void RenderContext::flush(bool synchronous) {
        if(synchronous){
            glFinish();
        }else{
            glFlush();
        }
    }

}

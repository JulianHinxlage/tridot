//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "RenderContext.h"
#include "core/core.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace tri {

    thread_local void *context = nullptr;

    void *RenderContext::create() {
        //init glfw
        static bool glfwInited = false;
        if(!glfwInited){
            if(glfwInit() != GLFW_TRUE) {
                env->console->error("failed to initialize GLFW");
                return nullptr;
            }
            glfwInited = true;
        }

        //create context
        GLFWwindow *c = glfwCreateWindow(1, 1, "", nullptr, (GLFWwindow*)context);
        if(!c){
            env->console->error("failed to create GLFW context");
            return nullptr;
        }
        set(c);

        //init glew
        static bool glewInited = false;
        if(!glewInited){
            if(glewInit() != GLEW_OK) {
                env->console->error("failed to initialize GLEW");
                return nullptr;
            }
            glewInited = true;
        }

        env->console->info("OpenGL version: ", glGetString(GL_VERSION));
        env->console->info("GLSL version: ", glGetString(GL_SHADING_LANGUAGE_VERSION));
        env->console->info("OpenGL vendor: ", glGetString(GL_VENDOR));
        env->console->info("OpenGL renderer: ", glGetString(GL_RENDERER));
        return c;
    }

    void *RenderContext::get() {
        return context;
    }

    void RenderContext::set(void *c) {
        if(c != context){
            glfwMakeContextCurrent((GLFWwindow*)c);
            context = c;
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

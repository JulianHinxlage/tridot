//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Window.h"
#include "FrameBuffer.h"
#include "core/Profiler.h"
#include "core/core.h"
#include "RenderContext.h"
#include "GL/glew.h"
#include "GLFW/glfw3.h"

namespace tri {

    TRI_REGISTER_SYSTEM_INSTANCE(Window, env->window);

    Window::Window() {
        size = {0, 0};
        position = {0, 0};
        vsync = true;
        backgroundColor = Color::white;
        context = nullptr;
    }

    Window::Window(int width, int height, const std::string &title) {
        size = {0, 0};
        position = {0, 0};
        vsync = true;
        backgroundColor = Color::white;
        context = nullptr;
        init(width, height, title);
    }

    Window::~Window() {
        clear();
    }

    void Window::bind() {
        FrameBuffer::unbind();
        RenderContext::set(context);
        if(context != nullptr){
            glViewport(0, 0, size.x, size.y);
        }
    }

    void Window::unbind() {
        RenderContext::set(nullptr);
    }

    void Window::init(int width, int height, const std::string &title, bool fullscreen) {
        TRI_PROFILE("window init");
        context = RenderContext::create();
        if(!context){
            return;
        }
        GLFWwindow *window = (GLFWwindow*)context;
        glfwSetWindowTitle(window, title.c_str());
        glfwSetWindowSize(window, width, height);
        if(fullscreen){
            GLFWmonitor *monitor = glfwGetPrimaryMonitor();
            glfwSetWindowMonitor(window, monitor, 0, 0, width, height, 0);
        }

        //setup callbacks
        glfwSetWindowUserPointer(window, this);
        glfwSetWindowSizeCallback(window, [](GLFWwindow *context, int width, int height){
            Window *window = (Window*)glfwGetWindowUserPointer(context);
            window->size = {width, height};
            glViewport(0, 0, width, height);
            env->console->trace("window resized: width = ", width, ", height = ", height);
        });
        glfwSetWindowPosCallback(window, [](GLFWwindow* context, int x, int y){
            Window *window = (Window*)glfwGetWindowUserPointer(context);
            window->position = glm::vec2(x, y);
            env->console->trace("window moved: x = ", x, ", y = ", y);
        });

        //set initial size and position
        int x = 0;
        int y = 0;
        glfwGetWindowPos(window, &x, &y);
        position = {x, y};
        glfwGetWindowSize(window, &x, &y);
        size =  {x, y};
    }

    void Window::update() {
        if(context != nullptr) {
            GLFWwindow *window = (GLFWwindow*)context;
            bind();
            glfwPollEvents();
            if (glfwWindowShouldClose(window)) {
                clear();
                return;
            }
            if (vsync) {
                glfwSwapInterval(1);
            } else {
                glfwSwapInterval(0);
            }
            {
                TRI_PROFILE("swap buffers");
                glfwSwapBuffers(window);
            }
            {
                TRI_PROFILE("clear");
                glm::vec4 color = backgroundColor.vec();
                glClearColor(color.r, color.g, color.b, color.a);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }
        }else{
            env->console->warning("window: update called before init or after close");
        }
    }

    bool Window::isOpen() {
        if(context != nullptr){
            if(glfwWindowShouldClose((GLFWwindow*)context) == GLFW_FALSE){
                return true;
            }else{
                clear();
                return false;
            }
        }else{
            return false;
        }
    }

    void Window::close() {
        if (context) {
            glfwSetWindowShouldClose((GLFWwindow*)context, true);
        }
    }

    void Window::clear() {
        if(context != nullptr) {
            void *current = RenderContext::get();
            RenderContext::set(context);
            RenderContext::destroy();
            if(current != context){
                RenderContext::set(current);
            }
            context = nullptr;
            env->console->trace("window closed");
        }
    }

    const glm::vec2 &Window::getSize()  const{
        return size;
    }

    const glm::vec2 &Window::getPosition()  const{
        return position;
    }

    const Color &Window::getBackgroundColor()  const{
        return backgroundColor;
    }

    bool Window::getVSync()  const{
        return vsync;
    }

    float Window::getAspectRatio() const {
        return size.x / size.y;
    }

    void *Window::getContext()  const{
        return context;
    }

    void Window::setSize(const glm::vec2 &size) {
        this->size = size;
        glfwSetWindowSize((GLFWwindow*)context, size.x, size.y);
    }

    void Window::setPosition(const glm::vec2 &position) {
        this->position = position;
        glfwSetWindowPos((GLFWwindow*)context, position.x, position.y);
    }

    void Window::setBackgroundColor(const Color &color) {
        this->backgroundColor = color;
    }

    void Window::setTitle(const std::string &title) {
        glfwSetWindowTitle((GLFWwindow*)context, title.c_str());
    }

    void Window::setVSync(bool enabled) {
        this->vsync = enabled;
    }

}

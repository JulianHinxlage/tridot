//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Window.h"
#include "FrameBuffer.h"
#include "tridot/core/Log.h"
#include "GL/glew.h"
#include "GLFW/glfw3.h"

namespace tridot {

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
        if(context != nullptr){
            clear();
        }
    }

    void bindWindow(GLFWwindow* window){
        static GLFWwindow* currentWindow = nullptr;
        if(window != currentWindow){
            glfwMakeContextCurrent(window);
            currentWindow = window;
        }
    }

    void Window::bind() {
        FrameBuffer::unbind();
        bindWindow((GLFWwindow*)context);
        if(context != nullptr){
            glViewport(0, 0, size.x, size.y);
        }
    }

    void Window::unbind() {
        bindWindow(nullptr);
    }

    void Window::init(int width, int height, const std::string &title) {
        //init glfw
        if(glfwInit() != GLFW_TRUE) {
            Log::error("failed to initialize GLFW");
            return;
        }

        //create window context
        GLFWwindow *window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if(!window){
            Log::error("failed to create GLFW window");
            return;
        }
        bindWindow(window);

        //init glew
        if(glewInit() != GLEW_OK) {
            Log::error("failed to initialize GLEW");
            return;
        }

        //setup callbacks
        glfwSetWindowUserPointer(window, this);
        glfwSetWindowSizeCallback(window, [](GLFWwindow *context, int width, int height){
            Window *window = (Window*)glfwGetWindowUserPointer(context);
            window->size = {width, height};
            glViewport(0, 0, width, height);
            Log::trace("window resized: width = ", width, ", height = ", height);
        });
        glfwSetWindowPosCallback(window, [](GLFWwindow* context, int x, int y){
            Window *window = (Window*)glfwGetWindowUserPointer(context);
            window->position = glm::vec2(x, y);
            Log::trace("window moved: x = ", x, ", y = ", y);
        });

        //set initial size and position
        int x = 0;
        int y = 0;
        glfwGetWindowPos(window, &x, &y);
        position = {x, y};
        glfwGetWindowSize(window, &x, &y);
        size =  {x, y};

        context = (void*)window;
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

            glfwSwapBuffers(window);
            glm::vec4 color = backgroundColor.vec();
            glClearColor(color.r, color.g, color.b, color.a);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }else{
            Log::warning("window: update called before init or after close");
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
        glfwSetWindowShouldClose((GLFWwindow*)context, true);
    }

    void Window::clear() {
        if(context != nullptr) {
            glfwDestroyWindow((GLFWwindow *) context);
            context = nullptr;
            Log::trace("window closed");
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

    void Window::setVSync(bool enabled) {
        this->vsync = enabled;
    }

}

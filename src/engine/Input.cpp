//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Input.h"
#include "render/Window.h"
#include "render/RenderThread.h"
#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>

namespace tri {

    TRI_REGISTER_SYSTEM_INSTANCE(Input, env->input);

    Input::Input() {
        wheel = 0;
        wheelUpdate = 0;
        allowInputs = true;
    }

    void Input::startup() {
        GLFWwindow* window = (GLFWwindow*)env->window->getContext();
        static Input *input = this;
        glfwSetScrollCallback(window, [](GLFWwindow *window, double x, double y){
            input->wheelUpdate += (float)y;
        });
    }

    void Input::update() {
        env->renderThread->addTask([&]() {

        GLFWwindow* window = (GLFWwindow*)env->window->getContext();
        if(!window){
            return;
        }
        for(auto &key : keys){
            key.second.pressed = false;
            key.second.released = false;
            if(allowInputs && glfwGetKey(window, key.first) == GLFW_PRESS){
                if(!key.second.down){
                    key.second.pressed = true;
                }
                key.second.down = true;
            }else{
                if(key.second.down){
                    key.second.released = true;
                }
                key.second.down = false;
            }
        }
        for(auto &button : buttons){
            button.second.pressed = false;
            button.second.released = false;
            if(allowInputs && glfwGetMouseButton(window, button.first) == GLFW_PRESS){
                if(!button.second.down){
                    button.second.pressed = true;
                }
                button.second.down = true;
            }else{
                if(button.second.down){
                    button.second.released = true;
                }
                button.second.down = false;
            }
        }

        //mouse position
        if (window) {
            double x = 0;
            double y = 0;
            glfwGetCursorPos(window, &x, &y);
            mousePosition = { x, y };
            if (mousePositionUpdate != glm::vec2(0, 0)) {
                glfwSetCursorPos(window, mousePositionUpdate.x, mousePositionUpdate.y);
                mousePositionUpdate = { 0, 0 };
            }
        }

        if (allowInputs) {
            wheel = wheelUpdate;
        }
        else {
            wheel = 0;
        }
        wheelUpdate = 0;

        });
    }

    Input::State Input::get(Input::Key key) {
        return keys[key];
    }

    Input::State Input::get(char key) {
        return keys[(Key)key];
    }

    Input::State Input::get(const char *key) {
        return keys[(Key)*key];
    }

    Input::State Input::get(Input::Button button) {
        return buttons[button];
    }

    bool Input::down(Input::Key key) {
        return get(key).down;
    }

    bool Input::down(char key) {
        return get(key).down;
    }

    bool Input::down(const char *key) {
        return get(key).down;
    }

    bool Input::down(Input::Button button) {
        return get(button).down;
    }

    bool Input::pressed(Input::Key key) {
        return get(key).pressed;
    }

    bool Input::pressed(char key) {
        return get(key).pressed;
    }

    bool Input::pressed(const char *key) {
        return get(key).pressed;
    }

    bool Input::pressed(Input::Button button) {
        return get(button).pressed;
    }

    bool Input::released(Input::Key key) {
        return get(key).released;
    }

    bool Input::released(char key) {
        return get(key).released;
    }

    bool Input::released(const char *key) {
        return get(key).released;
    }

    bool Input::released(Input::Button button) {
        return get(button).released;
    }

    glm::vec2 Input::getMousePosition(bool screenSpace) {
        if (screenSpace) {
            glm::vec2 m = mousePosition;
            m.x /= env->window->getSize().x;
            m.y /= env->window->getSize().y;
            m.y = 1.0f - m.y;
            m *= 2.0f;
            m -= glm::vec2(1, 1);
            return m;
        }
        else {
            return mousePosition;
        }
    }

    float Input::getMouseWheelDelta() {
        return wheel;
    }

    void Input::setMousePosition(glm::vec2 position, bool screenSpace) {
        if (screenSpace) {
            glm::vec2 m = position;
            m += glm::vec2(1, 1);
            m /= 2.0f;
            m.y = 1.0f - m.y;
            m.x *= env->window->getSize().x;
            m.y *= env->window->getSize().y;
            mousePositionUpdate = m;
        }
        else {
            mousePositionUpdate = position;
        }
    }

}
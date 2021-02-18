//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/core/config.h"
#include "tridot/render/Window.h"
#include "tridot/render/Shader.h"
#include "tridot/render/VertexArray.h"
#include "tridot/render/Texture.h"
#include "tridot/render/FrameBuffer.h"
#include "tridot/render/Camera.h"
#include "tridot/render/Mesh.h"
#include "tridot/render/MeshFactory.h"
#include "tridot/engine/Input.h"
#include "GL/gl.h"
#include "GLFW/glfw3.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>

using namespace tridot;

class Time{
public:
    float deltaTime;

    Time(){
        lastTime = 0;
        deltaTime = 0;
    }

    void update(){
        double t = glfwGetTime();
        deltaTime = t - lastTime;
        lastTime = t;
    }

private:
    double lastTime;
};
Time timer;

Input input;

void cameraController(PerspectiveCamera &cam, bool look, bool lockUp, float speed);

int main(int argc, char *argv[]){
    Log::options.logLevel = Log::TRACE;
    Log::info("Tridot version ", TRI_VERSION);
    Window window;
    window.init(800, 600, "Tridot " TRI_VERSION);
    input.init();

    Shader shader;
    shader.load("../res/shaders/shader.glsl");

    //Ref<Mesh> mesh = MeshFactory::createRegularPolygon(128);
    //Ref<Mesh> mesh = MeshFactory::createCube();
    Ref<Mesh> mesh(true);
    mesh->rescale = true;
    mesh->load("../res/models/teapot.obj");

    Ref<Mesh> cube = MeshFactory::createCube();
    Ref<Mesh> quad = MeshFactory::createQuad();

    Texture texture;
    texture.load("../res/textures/checkerboard.png");

    glm::vec2 pos(0, 0);
    glm::vec2 vel(0.1, 0.05);
    vel *= 0;

    FrameBuffer fbo;
    fbo.resize(window.getSize().x, window.getSize().y);
    fbo.setTexture(COLOR);
    fbo.setTexture(DEPTH);
    glEnable(GL_DEPTH_TEST);

    PerspectiveCamera camera;

    camera.position.x = 1;
    camera.forward.x = -1;

    bool look = true;
    bool lockUp = false;
    bool wireframe = false;

    while(window.isOpen()){
        if(input.pressed(tridot::Input::KEY_ESCAPE)){
            window.close();
        }

        timer.update();
        input.update();
        pos += vel * timer.deltaTime;

        fbo.bind();
        if(fbo.getSize() != window.getSize()){
            fbo.resize(window.getSize().x, window.getSize().y);
        }
        fbo.clear(window.getBackgroundColor());

        camera.aspectRatio = window.getAspectRatio();

        if(input.pressed('C')){
            look = !look;
        }
        if(input.pressed('X')){
            lockUp = !lockUp;
        }
        if(input.pressed('B')){
            wireframe = !wireframe;
        }

        cameraController(camera, look, lockUp, 2);

        if(wireframe){
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }else{
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        shader.bind();
        shader.set("uTransform", glm::translate(glm::mat4(1), glm::vec3(pos, 0)));
        shader.set("uProjection", camera.getProjection());
        shader.set("uTexture", 0);
        shader.set("uColor", glm::vec4(0.5, 0.8, 0.5, 1.0));
        texture.bind(0);
        mesh->vertexArray.submit();

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        fbo.unbind();
        fbo.getTexture(COLOR)->bind(0);
        shader.set("uTransform", glm::scale(glm::mat4(1), glm::vec3(2, 2, 1)));
        shader.set("uProjection", glm::mat4(1));
        shader.set("uColor", glm::vec4(1.0, 1.0, 1.0, 1.0));
        quad->vertexArray.submit();

        window.update();
    }
    return 0;
}

void cameraController(PerspectiveCamera &cam, bool look, bool lockUp, float speed){
    GLFWwindow  *window = glfwGetCurrentContext();
    speed *= timer.deltaTime;
    if(input.down('W')){
        cam.position += cam.forward * speed;
    }
    if(input.down('S')){
        cam.position -= cam.forward * speed;
    }
    if(input.down('D')){
        cam.position += cam.right * speed;
    }
    if(input.down('A')){
        cam.position -= cam.right * speed;
    }
    if(input.down('R')){
        cam.position += cam.up * speed;
    }
    if(input.down('F')){
        cam.position -= cam.up * speed;
    }

    float angle = 0;
    if(input.down('E')){
        angle += 1;
    }
    if(input.down('Q')){
        angle -= 1;
    }
    cam.up = glm::rotate(glm::mat4(1), angle * (float)timer.deltaTime, cam.forward) * glm::vec4(cam.up, 1.0f);

    //look
    static bool lastLook = false;
    if(look) {
        static glm::vec2 mousePositionLast = {0, 0};

        if (window != nullptr) {
            double x = 0;
            double y = 0;

            if(!lastLook){
                int width = 0;
                int height = 0;
                glfwGetWindowSize(window, &width, &height);
                glfwSetCursorPos(window, width / 2.0f, height / 2.0f);
                glfwGetCursorPos(window, &x, &y);
                mousePositionLast = {x, y};
            }

            glfwGetCursorPos(window, &x, &y);
            glm::vec2 mousePosition = {x, y};
            mousePosition = mousePosition - mousePositionLast;
            mousePosition /= 750;

            if (!lockUp) {
                cam.forward = glm::rotate(glm::mat4(1), -mousePosition.x, cam.up) * glm::vec4(cam.forward, 1.0f);
                glm::vec3 axis = glm::cross(cam.up, cam.forward);
                cam.forward = glm::rotate(glm::mat4(1), mousePosition.y, axis) * glm::vec4(cam.forward, 1.0f);
                cam.up = glm::rotate(glm::mat4(1), mousePosition.y, axis) * glm::vec4(cam.up, 1.0f);
            } else {
                cam.forward = glm::rotate(glm::mat4(1), -mousePosition.x, {0, 0, 1}) * glm::vec4(cam.forward, 1.0f);

                glm::vec3 dir = cam.forward;
                dir.z = 0;
                if(dir.x == 0 && dir.y == 0 && dir.z == 0){
                    dir = {1, 0, 0};
                }
                dir = glm::normalize(dir);
                float sign = cam.forward.z >= 0 ? 1 : -1;
                float angle = glm::angle(dir, cam.forward) * sign;
                glm::vec3 right = glm::cross({0, 0, 1}, dir);

                angle -= mousePosition.y;
                angle = std::min(angle, glm::radians(89.0f));
                angle = std::max(angle, glm::radians(-89.0f));
                cam.forward = glm::rotate(glm::mat4(1), -angle, right) * glm::vec4(dir, 1.0f);
                cam.up = glm::rotate(glm::mat4(1), -angle, right) * glm::vec4(0, 0, 1, 1.0f);
            }

            int width = 0;
            int height = 0;
            glfwGetWindowSize(window, &width, &height);

            glfwSetCursorPos(window, width / 2.0f, height / 2.0f);
            glfwGetCursorPos(window, &x, &y);
            mousePositionLast = {x, y};
        }
    }
    lastLook = look;
}



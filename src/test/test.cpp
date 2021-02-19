//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/core/config.h"
#include "tridot/render/Window.h"
#include "tridot/render/VertexArray.h"
#include "tridot/render/Texture.h"
#include "tridot/render/FrameBuffer.h"
#include "tridot/render/Camera.h"
#include "tridot/render/Mesh.h"
#include "tridot/render/MeshRenderer.h"
#include "tridot/engine/Input.h"
#include "tridot/engine/Time.h"
#include "GL/gl.h"
#include "GLFW/glfw3.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>

using namespace tridot;

Time timer;
Input input;

void cameraController(PerspectiveCamera &cam, bool look, bool lockUp, float speed);

class Entity{
public:
    glm::vec3 pos;
    glm::vec3 rot;
    glm::vec3 scale;
    glm::vec3 vel;
    glm::vec3 angular;
    Color color;

    void update(){
        pos += vel * timer.deltaTime;
        rot += angular * timer.deltaTime;
    }
};

float randu(){
    return (double)std::rand() / (double)RAND_MAX;
}

glm::vec2 randu2(){
    return glm::vec2(randu(), randu());
}
glm::vec3 randu3(){
    return glm::vec3(randu(), randu(), randu());
}
glm::vec4 randu4(){
    return glm::vec4(randu(), randu(), randu(), randu());
}

int main(int argc, char *argv[]){
    Log::addTarget("log.txt", {Log::TRACE, true, true, false});
    Log::info("Tridot version ", TRI_VERSION);
    Log::options.logLevel = Log::TRACE;


    Window window;
    window.init(800, 600, "Tridot " TRI_VERSION);
    input.init();

    MeshRenderer renderer;
    renderer.init();

    Ref<Mesh> mesh(true);
    mesh->rescale = true;
    mesh->load("../res/models/teapot.obj");

    Texture texture;
    texture.load("../res/textures/checkerboard.png");

    FrameBuffer fbo;
    fbo.resize(window.getSize().x, window.getSize().y);
    fbo.setTexture(COLOR);
    fbo.setTexture(DEPTH);
    glEnable(GL_DEPTH_TEST);

    PerspectiveCamera camera;
    camera.position.z = 1;
    camera.forward.z = -1;

    std::vector<Entity> entities;

    int area = 40;
    for(int i = 0; i < 5000; i++){
        entities.push_back({});
        Entity &e = entities.back();
        e.pos = (randu3() - 0.5f) * (float)area;
        e.vel = (randu3() - 0.5f) * 0.5f;
        e.scale = glm::vec3(1, 1, 1) * (randu() * 0.5f + 0.5f);
        e.rot = randu3();
        e.angular = (randu3() - 0.5f) * 0.5f;
        e.color = Color(glm::vec4(randu3() * 0.6f + 0.2f, 1.0));
    }

    window.setBackgroundColor(Color(100, 100, 100));

    bool look = true;
    bool lockUp = false;
    bool wireframe = false;

    while(window.isOpen()){
        if(input.pressed(tridot::Input::KEY_ESCAPE)){
            window.close();
        }

        timer.update();
        input.update();
        if(timer.frameTicks(1.0)){
            Log::info(timer.framesPerSecond, " fps, ", timer.avgFrameTime * 1000, " ms [", timer.minFrameTime * 1000, ", ", timer.maxFrameTime * 1000, "]");
        }

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
        if(input.pressed('V')){
            window.setVSync(!window.getVSync());
        }
        cameraController(camera, look, lockUp, 2);

        if(wireframe){
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }else{
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        renderer.begin(camera.getProjection(), &fbo);
        for(auto &e : entities){
            e.update();
            renderer.submit({e.pos, e.scale, e.rot, e.color}, &texture, mesh.get());
        }
        renderer.end();

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        renderer.begin(glm::mat4(1), nullptr);
        renderer.submit({{0, 0, 0}, {2, 2, 1}}, fbo.getTexture(COLOR).get());
        renderer.end();

        glFinish();
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



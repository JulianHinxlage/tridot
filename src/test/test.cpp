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
#include "tridot/render/MeshFactory.h"
#include "tridot/engine/Input.h"
#include "tridot/engine/Time.h"
#include "tridot/engine/ResourceLoader.h"
#include "tridot/engine/Physics.h"
#include "GL/gl.h"
#include "GLFW/glfw3.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>

using namespace tridot;

Time timer;
Input input;

class Entity{
public:
    Transform t;
    RigidBody rb;
    Collider collider;
    Color color;
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

void cameraController(PerspectiveCamera &cam, bool look, bool lockUp, float speed);

void createPhysicsTest(std::vector<Entity> &entities);

int main(int argc, char *argv[]){
    //logging
    Log::options.logLevel = Log::TRACE;
    Log::info("Tridot version ", TRI_VERSION);

    //window, input, time
    Window window;
    window.init(800, 600, "Tridot " TRI_VERSION);
    window.setBackgroundColor(Color(100, 100, 100));
    input.init();
    timer.init();

    //physics
    Physics physics;
    physics.init({0, 0, -10});

    //resource loader
    ResourceLoader resources;
    resources.autoReload = true;
    resources.addSearchDirectory("../res");
    resources.addSearchDirectory("../res/textures");
    resources.addSearchDirectory("../res/models");
    resources.addSearchDirectory("../res/shaders");

    //rendering
    MeshRenderer renderer;
    renderer.init(resources.get<Shader>("mesh.glsl"), 1000);
    Ref<Shader> boxShader = resources.get<Shader>("meshTextureScale.glsl");
    Ref<Shader> shader = resources.get<Shader>("mesh.glsl");
    Ref<Mesh> cube = MeshFactory::createCube();
    Ref<Mesh> sphere = MeshFactory::createSphere(32, 32);
    Ref<Texture> texture = resources.get<Texture>("checkerboard.png");

    //framebuffer
    FrameBuffer frameBuffer;
    frameBuffer.resize(window.getSize().x, window.getSize().y);
    frameBuffer.setTexture(COLOR);
    frameBuffer.setTexture(DEPTH);
    glEnable(GL_DEPTH_TEST);

    //camera
    PerspectiveCamera camera;
    camera.position = {0, 20, 0};
    camera.forward = {0, -1, 0};

    //create scene
    std::vector<Entity> entities;
    createPhysicsTest(entities);

    //camera mode
    bool look = true;
    bool lockUp = true;
    float speed = 10;
    bool wireframe = false;

    while(window.isOpen()){
        if(input.pressed(tridot::Input::KEY_ESCAPE)){
            window.close();
        }

        //update systems
        timer.update();
        input.update();
        resources.update();
        physics.step(timer.deltaTime);

        //fps info
        if(timer.frameTicks(1.0)){
            Log::info(timer.framesPerSecond, " fps, ", timer.avgFrameTime * 1000, " ms [", timer.minFrameTime * 1000, ", ", timer.maxFrameTime * 1000, "]");
        }

        //clear framebuffer
        frameBuffer.bind();
        if(frameBuffer.getSize() != window.getSize()){
            frameBuffer.resize(window.getSize().x, window.getSize().y);
        }
        frameBuffer.clear(window.getBackgroundColor());

        //camera
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
        speed *= std::pow(1.3f, input.mouseWheelDelta());
        cameraController(camera, look, lockUp, speed);

        //shoot spheres
        if(input.pressed(Input::MOUSE_BUTTON_LEFT)){
            entities.push_back({});
            Entity &e = entities.back();
            e.color = Color(glm::vec4(0.3, 0.3, 0.3, 1.0));
            e.t.position = camera.position + camera.forward * 1.5f;
            e.rb.mass = 200;
            e.rb.velocity = camera.forward * 300.0f;
            e.collider.type = Collider::SPHERE;
        }

        //rendering
        if(wireframe){
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }else{
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        renderer.begin(camera.getProjection(), &frameBuffer);
        for(int i = 0; i < entities.size(); i++){
            auto &e = entities[i];
            Mesh *mesh = e.collider.type == Collider::SPHERE ? sphere.get() : cube.get();
            Shader *s = e.collider.type == Collider::SPHERE ? shader.get() : boxShader.get();
            physics.update(e.rb, e.t, e.collider, i);
            renderer.submit({e.t.position, e.t.scale, e.t.rotation, e.color, {0, 0}, {0.5, 0.5}}, texture.get(), mesh, s);
        }
        renderer.end();

        //draw frame buffer
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        renderer.begin(glm::mat4(1), nullptr);
        renderer.submit({{0, 0, 0}, {2, 2, 1}}, frameBuffer.getTexture(COLOR).get());
        renderer.end();

        //window
        glFinish();
        window.update();
    }
    return 0;
}

void createPhysicsTest(std::vector<Entity> &entities){
    int area = 10;

    //ground
    {
        entities.push_back({});
        Entity &e = entities.back();
        e.color = Color::white * 0.7f;
        e.t.scale = {100, 100, 1};
        e.t.position = {0, 0, -1};
        e.rb.mass = 0;
    }

    int wallSize = 4;
    int wallDistance = 16;
    //walls
    {
        for(int i = 0; i < 4; i++) {
            entities.push_back({});
            Entity &e = entities.back();
            if (i < 2) {
                e.color = Color::white * 0.7f;
                e.t.scale = {1, wallDistance + 1, wallSize};
                e.t.position = {i % 2 == 0 ? wallDistance / 2 : -wallDistance / 2, 0, wallSize / 2 - 0.5};
                e.rb.mass = 0;
            } else if (i < 4) {
                e.color = Color::white * 0.7f;
                e.t.scale = {18, 1, 7};

                e.t.scale = {wallDistance + 1, 1, wallSize};
                e.t.position = {0, i % 2 == 0 ? wallDistance / 2 : -wallDistance / 2, wallSize / 2 - 0.5};

                e.t.position += glm::vec3(0.001, 0.001, 0.001);
                e.rb.mass = 0;
            }
        }
    }

    wallSize = 4;
    wallDistance = 100;
    //walls
    {
        for(int i = 0; i < 4; i++) {
            entities.push_back({});
            Entity &e = entities.back();
            if (i < 2) {
                e.color = Color::white * 0.7f;
                e.t.scale = {1, wallDistance + 1, wallSize};
                e.t.position = {i % 2 == 0 ? wallDistance / 2 : -wallDistance / 2, 0, wallSize / 2 - 0.5};
                e.rb.mass = 0;
            } else if (i < 4) {
                e.color = Color::white * 0.7f;
                e.t.scale = {18, 1, 7};

                e.t.scale = {wallDistance + 1, 1, wallSize};
                e.t.position = {0, i % 2 == 0 ? wallDistance / 2 : -wallDistance / 2, wallSize / 2 - 0.5};
                e.t.position += glm::vec3(0.001, 0.001, 0.001);
                e.rb.mass = 0;
            }
        }
    }

    //entities
    for(int i = 0; i < area * area * area; i++){
        entities.push_back({});
        Entity &e = entities.back();
        e.color = Color(glm::vec4(randu3() * 0.6f + 0.2f, 1.0));
        e.t.position.x = i / (area * area) - (area / 2);
        e.t.position.y = i / (area) % area - (area / 2);
        e.t.position.z = i % area;
        e.collider.type = (i % 2 == 0) ? Collider::SPHERE : Collider::BOX;
    }
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
                glm::vec3 up = {0, 0, 1};

                cam.forward = glm::rotate(glm::mat4(1), -mousePosition.x, up) * glm::vec4(cam.forward, 1.0f);

                glm::vec3 dir = cam.forward;
                dir.z = 0;
                if(dir.x == 0 && dir.y == 0 && dir.z == 0){
                    dir = {1, 0, 0};
                }
                dir = glm::normalize(dir);
                float sign = cam.forward.z >= 0 ? 1 : -1;
                float angle = glm::angle(dir, cam.forward) * sign;
                glm::vec3 right = glm::cross(up, dir);

                angle -= mousePosition.y;
                angle = std::min(angle, glm::radians(89.0f));
                angle = std::max(angle, glm::radians(-89.0f));
                cam.forward = glm::rotate(glm::mat4(1), -angle, right) * glm::vec4(dir, 1.0f);
                cam.up = glm::rotate(glm::mat4(1), -angle, right) * glm::vec4(up, 1.0f);
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

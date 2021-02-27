//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/engine/Engine.h"
#include "tridot/render/Camera.h"
#include "tridot/render/MeshFactory.h"
#include "tridot/components/Transform.h"
#include "tridot/components/RenderComponent.h"
#include "tridot/ecs/Prefab.h"
#include <GLFW/glfw3.h>
#include <glm/gtx/vector_angle.hpp>

using namespace tridot;
using namespace ecs;

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

void playerControl(EntityId playerId, PerspectiveCamera &camera);
void createScene();
void cameraControl(PerspectiveCamera &cam, bool move, bool look, bool lockUp, float speed);

int main(int argc, char *argv[]){
    Log::options.logLevel = Log::TRACE;
    engine.init(800, 600, "Tridot " TRI_VERSION, "../res/", true);
    engine.window.setBackgroundColor(Color::white);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    createScene();

    //create player
    Ref<Shader> shader = engine.resources.get<Shader>("mesh.glsl");
    EntityId playerId = engine.create(
            Transform({0, 0, 0}, {1, 1, 1}),
            RenderComponent(glm::vec3(0.3, 0.6, 0.8), {2.0f, 2.0f})
            .setTexture(engine.resources.get<Texture>("tex1.png"))
            .setMesh(MeshFactory::createSphere(32, 32)).setShader(shader),
            RigidBody(5),
            Collider(Collider::SPHERE)
    );

    //create camera
    PerspectiveCamera &camera = engine.add<PerspectiveCamera>(engine.create());
    camera.position = {0, -2, 0};
    camera.forward = {0, 1, 0};
    camera.right = {1, 0, 0};
    camera.up = {0, 0, 1};
    camera.near = 0.05;
    camera.far = 1200;

    engine.onUpdate().add([&](){
        if(engine.input.pressed(Input::KEY_ESCAPE)){
            engine.window.close();
        }
        if(engine.time.frameTicks(0.5)){
            Log::info(engine.time.framesPerSecond, " fps, ", engine.time.avgFrameTime * 1000, "ms [", engine.time.minFrameTime * 1000, " ms, ", engine.time.maxFrameTime * 1000, " ms]");
        }
        if(engine.input.pressed('V')){
            engine.window.setVSync(!engine.window.getVSync());
        }

        engine.view<PerspectiveCamera>().each([&](PerspectiveCamera &camera){
            playerControl(playerId, camera);
            if(shader->has("uCameraPosition")){
                shader->set("uCameraPosition", camera.position);
            }
        });
    }, "camera");

    engine.run();
    return 0;
}

void playerControl(EntityId playerId, PerspectiveCamera &camera){
    float speed = 20;
    static float jumpBufferTimer = 0;
    static float cameraDistance = 5;

    RigidBody &rigidBody = engine.get<RigidBody>(playerId);
    Transform &transform = engine.get<Transform>(playerId);


    glm::vec2 in = {0, 0};
    if(engine.input.down('W')) {
        in.y += 1;
    }if(engine.input.down('S')) {
        in.y -= 1;
    }if(engine.input.down('A')) {
        in.x -= 1;
    }if(engine.input.down('D')) {
        in.x += 1;
    }

    if(in != glm::vec2(0, 0)){
        in = glm::normalize(in);
    }

    glm::vec3 dirX = camera.right;
    dirX.z = 0;
    dirX = glm::normalize(dirX);

    glm::vec3 dirY = camera.forward;
    dirY.z = 0;
    dirY = glm::normalize(dirY);


    rigidBody.velocity += (dirX * in.x + dirY * in.y) * speed * engine.time.deltaTime;

    if(in.x == 0 && in.y == 0){
        rigidBody.velocity.x *= std::pow(0.0001, engine.time.deltaTime);
        rigidBody.velocity.y *= std::pow(0.0001, engine.time.deltaTime);
    }else{
        rigidBody.velocity.x *= std::pow(0.1, engine.time.deltaTime);
        rigidBody.velocity.y *= std::pow(0.1, engine.time.deltaTime);
    }

    if(engine.input.pressed(engine.input.KEY_SPACE)) {
        jumpBufferTimer = 0.1;
    }

    if(jumpBufferTimer > 0){
        bool onGround = false;

        engine.physics.contacts(rigidBody, [playerId, &transform, &onGround](const glm::vec3 &pos, int index){
            if(index != playerId){
                if(pos.z < transform.position.z - 0.2)
                    onGround = true;
            }
        });

        if(onGround){
            rigidBody.velocity.z = 10;
        }
    }
    jumpBufferTimer -= engine.time.deltaTime;
    if(engine.input.released(engine.input.KEY_SPACE)){
        if(rigidBody.velocity.z > 0.1){
            rigidBody.velocity.z /= 2;
        }
    }

    if(rigidBody.velocity.z > 10){
        rigidBody.velocity.z = 10;
    }

    engine.physics.update(rigidBody, transform, engine.get<Collider>(playerId), playerId);

    static bool look = true;
    if(engine.input.pressed('C')){
        look = !look;
    }
    cameraControl(camera, true, look, true, 0);

    //camera
    cameraDistance /= std::pow(1.3f, engine.input.mouseWheelDelta());
    camera.position = transform.position - camera.forward * cameraDistance;
}

void createScene(){
    Ref<Texture> tex1 = engine.resources.get<Texture>("tex1.png");
    Ref<Texture> tex2 = engine.resources.get<Texture>("tex2.png");
    Ref<Texture> tex3 = engine.resources.get<Texture>("tex3.png");
    Ref<Texture> tex4 = engine.resources.get<Texture>("tex4.png");

    Ref<Mesh> cube = MeshFactory::createCube();
    Ref<Shader> shader = engine.resources.get<Shader>("mesh.glsl");

    int groundSize = 200;
    int wallHeight = 30;

    Prefab wall(
            Transform(),
            RenderComponent(Color::white * 0.7f).setTexture(tex3).setMesh(cube).setShader(shader),
            RigidBody(0),
            Collider(Collider::BOX)
    );


    //ground
    wall.instantiate(engine, Transform({0, 0, -1}, {groundSize, groundSize, 1}));


    //selling
    wall.instantiate(engine, Transform({0, 0, wallHeight}, {groundSize, groundSize, 1}));


    //walls
    for(int i = 0; i < 4; i++) {
        if (i < 2) {
            wall.instantiate(engine, Transform(
                    {i % 2 == 0 ? groundSize / 2 - 0.5 : -groundSize / 2 + 0.5, 0, wallHeight / 2 - 0.5},
                    {1, groundSize, wallHeight}
            ));
        } else if (i < 4) {
            wall.instantiate(engine, Transform(
                    {0, i % 2 == 0 ? groundSize / 2 - 0.5 : -groundSize / 2 + 0.5, wallHeight / 2 - 0.5},
                     {groundSize, 1, wallHeight}
            ));
        }
    }

    int platformCount = 0;
    for(int x = -groundSize / 2 + 2; x < groundSize / 2 - 1; x++){
        for(int y = -groundSize / 2 + 2; y < groundSize / 2 - 1; y++){

            if(randu() < 0.025) {
                int width = int(randu() * 5) + 2;
                int depth = int(randu() * 5) + 2;
                float height = (int(randu() * 10) + 1) * 2;
                height += x * 0.001 + y * 0.001;

                engine.create(
                        Transform({x, y, height}, {width, depth, 0.5}),
                        RenderComponent(randu3() * 0.0f + 0.5f).setTexture(tex1).setMesh(cube).setShader(shader),
                        RigidBody(0),
                        Collider(Collider::BOX)
                        );

                platformCount++;
            }
        }
    }
    Log::info("generated ", platformCount, " platforms");
}

void cameraControl(PerspectiveCamera &cam, bool move, bool look, bool lockUp, float speed){
    GLFWwindow  *window = glfwGetCurrentContext();
    speed *= engine.time.deltaTime;

    if(move) {
        if (engine.input.down('W')) {
            cam.position += cam.forward * speed;
        }
        if (engine.input.down('S')) {
            cam.position -= cam.forward * speed;
        }
        if (engine.input.down('D')) {
            cam.position += cam.right * speed;
        }
        if (engine.input.down('A')) {
            cam.position -= cam.right * speed;
        }
        if (engine.input.down('R')) {
            cam.position += cam.up * speed;
        }
        if (engine.input.down('F')) {
            cam.position -= cam.up * speed;
        }

        float angle = 0;
        if (engine.input.down('E')) {
            angle += 1;
        }
        if (engine.input.down('Q')) {
            angle -= 1;
        }
        cam.up = glm::rotate(glm::mat4(1), angle * (float) engine.time.deltaTime, cam.forward) * glm::vec4(cam.up, 1.0f);
    }

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

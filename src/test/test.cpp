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

#include "tridot/ecs/Registry.h"

using namespace tridot;

Time timer;
Input input;

class Entity{
public:
    Transform transform;
    RigidBody rigidBody;
    Collider collider;
    Color color;
    Texture *texture;
    glm::vec2 textureScale = {0.5, 0.5};
};

class Material{
public:
    Ref<Shader> shader;
    Color ambient;
    Color diffuse;
    Color specular;
    float shininess;
    bool textureWrapByScale = false;
};

class MaterialInstance{
    Ref<Material> material;
    Ref<Texture> texture;
    Color color = Color::white;
    glm::vec2 texCoordsBottomRight = {0, 0};
    glm::vec2 texCoordsTopLeft = {1, 1};
};

class DirectionalLight{
    glm::vec3 direction;
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

void cameraController(PerspectiveCamera &cam, bool move, bool look, bool lockUp, float speed);

void createTestScene(std::vector<Entity> &entities, ResourceLoader &resources);

void createPhysicsTest(std::vector<Entity> &entities, ResourceLoader &resources);

void shootSphere(std::vector<Entity> &entities, PerspectiveCamera &camera);

int main(int argc, char *argv[]){
    //logging
    Log::options.logLevel = Log::TRACE;
    Log::info("Tridot version ", TRI_VERSION);

    //window, input, time
    Window window;
    window.init(800, 600, "Tridot " TRI_VERSION);
    window.setBackgroundColor(Color::white);
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
    renderer.init(resources.get<Shader>("meshBase.glsl"), 1000);
    Ref<Shader> boxShader = resources.get<Shader>("meshTextureScale.glsl");
    Ref<Shader> shader = resources.get<Shader>("mesh.glsl");
    Ref<Shader> shadowShader = resources.get<Shader>("shadow.glsl");

    Ref<Mesh> cube = MeshFactory::createCube();
    Ref<Mesh> sphere = MeshFactory::createSphere(32, 32);
    Ref<Texture> tex1 = resources.get<Texture>("tex1.png");
    Ref<Texture> tex2 = resources.get<Texture>("tex2.png");
    Ref<Texture> tex3 = resources.get<Texture>("tex3.png");
    Ref<Texture> tex4 = resources.get<Texture>("tex4.png");

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
    camera.near = 0.05;
    camera.far = 1200;

    //create scene
    std::vector<Entity> entities;
    createTestScene(entities, resources);

    //create player
    int playerId = 0;
    {
        playerId = entities.size();
        entities.push_back({});
        Entity &e = entities.back();
        e.color = Color(glm::vec4(0.3, 0.6, 0.8, 1.0));
        e.transform.position = {0, 0, 0};
        e.rigidBody.mass = 1;
        e.rigidBody.velocity = {0, 0, 0};
        e.collider.type = Collider::SPHERE;
        e.rigidBody.friction = 1.0f;
        e.texture = tex4.get();
        e.textureScale = {0.5f, 0.5f};
        e.rigidBody.mass = 5;
    }

    //camera mode
    bool look = true;
    bool wireframe = false;
    float speed = 20;
    float cameraDistance = 5;
    float jumpBufferTimer = 0;
    Clock clock;

    while(window.isOpen()){
        if(input.pressed(tridot::Input::KEY_ESCAPE)){
            window.close();
        }

        //update systems
        timer.update();
        input.update();
        resources.update();

        //physics
        clock.reset();
        physics.step(timer.deltaTime);
        for(int i = 0; i < entities.size(); i++){
            auto &e = entities[i];
            physics.update(e.rigidBody, e.transform, e.collider, i);
        }

        //fps info
        if(timer.frameTicks(1.0)){
            Log::info(timer.framesPerSecond, " fps, ", timer.avgFrameTime * 1000, " ms [", timer.minFrameTime * 1000, ", ", timer.maxFrameTime * 1000, "]");
            Log::info("physics: ", clock.elapsed() * 1000, " ms");
        }

        //clear framebuffer
        frameBuffer.bind();
        if(frameBuffer.getSize() != window.getSize()){
            frameBuffer.resize(window.getSize().x, window.getSize().y);
        }
        frameBuffer.clear(window.getBackgroundColor());



        //player control
        Entity &player = entities[playerId];
        glm::vec2 in = {0, 0};
        if(input.down('W')) {
            in.y += 1;
        }if(input.down('S')) {
            in.y -= 1;
        }if(input.down('A')) {
            in.x -= 1;
        }if(input.down('D')) {
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

        player.rigidBody.velocity += (dirX * in.x + dirY * in.y) * speed * timer.deltaTime;

        if(in.x == 0 && in.y == 0){
            player.rigidBody.velocity.x *= std::pow(0.000001, timer.deltaTime);
            player.rigidBody.velocity.y *= std::pow(0.000001, timer.deltaTime);
        }else{
            player.rigidBody.velocity.x *= std::pow(0.1, timer.deltaTime);
            player.rigidBody.velocity.y *= std::pow(0.1, timer.deltaTime);
        }



        if(input.pressed(input.KEY_SPACE)) {
            jumpBufferTimer = 0.1;
        }

        if(jumpBufferTimer > 0){
            bool onGround = false;

            physics.contacts(player.rigidBody, [playerId, &player, &onGround](const glm::vec3 &pos, int index){
                if(index != playerId){
                    if(pos.z < player.transform.position.z - 0.2)
                    onGround = true;
                }
            });

            if(onGround){
                player.rigidBody.velocity.z = 10;
            }
        }
        jumpBufferTimer -= timer.deltaTime;
        if(input.released(input.KEY_SPACE)){
            if(player.rigidBody.velocity.z > 0.1){
                player.rigidBody.velocity.z /= 2;
            }
        }

        if(player.rigidBody.velocity.z > 10){
            player.rigidBody.velocity.z = 10;
        }

        physics.update(entities[playerId].rigidBody, entities[playerId].transform, entities[playerId].collider, playerId);

        //camera
        camera.aspectRatio = window.getAspectRatio();
        if(input.pressed('V')){
            window.setVSync(!window.getVSync());
        }
        if(input.pressed('B')){
            wireframe = !wireframe;
        }
        if(input.pressed('C')){
            look = !look;
        }
        cameraDistance /= std::pow(1.3f, input.mouseWheelDelta());
        cameraController(camera, false, look, true, speed);
        camera.position = player.transform.position - camera.forward * cameraDistance;

        //rendering
        clock.reset();
        if(wireframe){
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }else{
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        if(shader->getId() != 0){
            shader->bind();
            shader->set("uCameraPosition", camera.position);
        }
        renderer.begin(camera.getProjection(), &frameBuffer);
        for(int i = 0; i < entities.size(); i++){
            auto &e = entities[i];
            Mesh *mesh = e.collider.type == Collider::SPHERE ? sphere.get() : cube.get();
            Shader *s = e.collider.type == Collider::SPHERE ? shader.get() : boxShader.get();
            renderer.submit({e.transform.position, e.transform.scale, e.transform.rotation, e.color, {0, 0}, e.textureScale}, e.texture, mesh, s);
        }
        renderer.end();
        if(timer.frameTicks(1.0)){
            Log::info("render: ", clock.elapsed() * 1000, " ms");
        }

        //draw frame buffer
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        renderer.begin(glm::mat4(1), nullptr);
        renderer.submit({{0, 0, 0}, {2, 2, 1}}, frameBuffer.getTexture(COLOR).get());
        renderer.end();

        //window
        clock.reset();
        glFinish();
        window.update();
        if(timer.frameTicks(1.0)){
            Log::info("window: ", clock.elapsed() * 1000, " ms");
        }
    }
    return 0;
}

void createTestScene(std::vector<Entity> &entities, ResourceLoader &resources){

    Ref<Texture> tex1 = resources.get<Texture>("tex1.png");
    Ref<Texture> tex2 = resources.get<Texture>("tex2.png");
    Ref<Texture> tex3 = resources.get<Texture>("tex3.png");
    Ref<Texture> tex4 = resources.get<Texture>("tex4.png");

    int groundSize = 200;
    int wallHeight = 30;

    //ground
    {
        entities.push_back({});
        Entity &e = entities.back();
        e.color = Color::white * 0.7f;
        e.transform.scale = {groundSize, groundSize, 1};
        e.transform.position = {0, 0, -1};
        e.rigidBody.mass = 0;
        e.texture = tex3.get();
    }

    //selling
    {
        entities.push_back({});
        Entity &e = entities.back();
        e.color = Color::white * 0.7f;
        e.transform.scale = {groundSize, groundSize, 1};
        e.transform.position = {0, 0, wallHeight};
        e.rigidBody.mass = 0;
        e.texture = tex3.get();
    }

    //walls
    {
        for(int i = 0; i < 4; i++) {
            entities.push_back({});
            Entity &e = entities.back();
            if (i < 2) {
                e.color = Color::white * 0.7f;
                e.transform.scale = {1, groundSize, wallHeight};
                e.transform.position = {i % 2 == 0 ? groundSize / 2 - 0.5 : -groundSize / 2 + 0.5, 0, wallHeight / 2 - 0.5};
                e.rigidBody.mass = 0;
                e.texture = tex3.get();
            } else if (i < 4) {
                e.color = Color::white * 0.7f;
                e.transform.scale = {groundSize, 1, wallHeight};
                e.transform.position = {0, i % 2 == 0 ? groundSize / 2 - 0.5 : -groundSize / 2 + 0.5, wallHeight / 2 - 0.5};
                e.transform.position += glm::vec3(0.001, 0.001, 0.001);
                e.rigidBody.mass = 0;
                e.texture = tex3.get();
            }
        }
    }

    int platformCount = 0;
    for(int x = -groundSize / 2 + 2; x < groundSize / 2 - 1; x++){
        for(int y = -groundSize / 2 + 2; y < groundSize / 2 - 1; y++){

            if(randu() < 0.025) {
                float height = (int(randu() * 10) + 1) * 2;

                int width = int(randu() * 5) + 2;
                int depth = int(randu() * 5) + 2;


                height += x * 0.001 + y * 0.001;

                entities.push_back({});
                Entity &e = entities.back();
                e.color = Color(glm::vec4(randu3() * 0.0f + 0.5f, 1.0));
                e.transform.position.x = x;
                e.transform.position.y = y;
                e.transform.position.z = height;
                e.transform.scale = {width, depth, 0.5};
                e.collider.type = Collider::BOX;
                e.rigidBody.mass = 0;
                e.texture = tex1.get();
                e.textureScale = {1.0, 1.0};
                platformCount++;
            }
        }
    }
    Log::info("generated ", platformCount, " platforms");
}

void createPhysicsTest(std::vector<Entity> &entities, ResourceLoader &resources){
    int area = 8;
    Ref<Texture> tex = resources.get<Texture>("checkerboard.png");

    //ground
    {
        entities.push_back({});
        Entity &e = entities.back();
        e.color = Color::white * 0.7f;
        e.transform.scale = {100, 100, 1};
        e.transform.position = {0, 0, -1};
        e.rigidBody.mass = 0;
        e.texture = tex.get();
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
                e.transform.scale = {1, wallDistance + 1, wallSize};
                e.transform.position = {i % 2 == 0 ? wallDistance / 2 : -wallDistance / 2, 0, wallSize / 2 - 0.5};
                e.rigidBody.mass = 0;
                e.texture = tex.get();
            } else if (i < 4) {
                e.color = Color::white * 0.7f;
                e.transform.scale = {wallDistance + 1, 1, wallSize};
                e.transform.position = {0, i % 2 == 0 ? wallDistance / 2 : -wallDistance / 2, wallSize / 2 - 0.5};
                e.transform.position += glm::vec3(0.001, 0.001, 0.001);
                e.rigidBody.mass = 0;
                e.texture = tex.get();
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
                e.transform.scale = {1, wallDistance + 1, wallSize};
                e.transform.position = {i % 2 == 0 ? wallDistance / 2 : -wallDistance / 2, 0, wallSize / 2 - 0.5};
                e.rigidBody.mass = 0;
                e.texture = tex.get();
            } else if (i < 4) {
                e.color = Color::white * 0.7f;
                e.transform.scale = {18, 1, 7};
                e.transform.scale = {wallDistance + 1, 1, wallSize};
                e.transform.position = {0, i % 2 == 0 ? wallDistance / 2 : -wallDistance / 2, wallSize / 2 - 0.5};
                e.transform.position += glm::vec3(0.001, 0.001, 0.001);
                e.rigidBody.mass = 0;
                e.texture = tex.get();
            }
        }
    }

    //entities
    for(int i = 0; i < area * area * area; i++){
        entities.push_back({});
        Entity &e = entities.back();
        e.color = Color(glm::vec4(randu3() * 0.6f + 0.2f, 1.0));
        e.transform.position.x = i / (area * area) - (area / 2);
        e.transform.position.y = i / (area) % area - (area / 2);
        e.transform.position.z = i % area;
        e.collider.type = (i % 2 == 0) ? Collider::SPHERE : Collider::BOX;
        e.texture = tex.get();
    }
}

void shootSphere(std::vector<Entity> &entities, PerspectiveCamera &camera){
    //shoot spheres
    if(input.pressed(Input::MOUSE_BUTTON_LEFT)){
        entities.push_back({});
        Entity &e = entities.back();
        e.color = Color(glm::vec4(0.3, 0.3, 0.3, 1.0));
        e.transform.position = camera.position + camera.forward * 1.5f;
        e.rigidBody.mass = 200;
        e.rigidBody.velocity = camera.forward * 300.0f;
        e.collider.type = Collider::SPHERE;
    }
}

void cameraController(PerspectiveCamera &cam, bool move, bool look, bool lockUp, float speed){
    GLFWwindow  *window = glfwGetCurrentContext();
    speed *= timer.deltaTime;

    if(move) {
        if (input.down('W')) {
            cam.position += cam.forward * speed;
        }
        if (input.down('S')) {
            cam.position -= cam.forward * speed;
        }
        if (input.down('D')) {
            cam.position += cam.right * speed;
        }
        if (input.down('A')) {
            cam.position -= cam.right * speed;
        }
        if (input.down('R')) {
            cam.position += cam.up * speed;
        }
        if (input.down('F')) {
            cam.position -= cam.up * speed;
        }

        float angle = 0;
        if (input.down('E')) {
            angle += 1;
        }
        if (input.down('Q')) {
            angle -= 1;
        }
        cam.up = glm::rotate(glm::mat4(1), angle * (float) timer.deltaTime, cam.forward) * glm::vec4(cam.up, 1.0f);
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

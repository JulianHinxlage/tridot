//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/engine/Engine.h"
#include "tridot/render/Camera.h"
#include "tridot/render/MeshFactory.h"
#include "tridot/components/Transform.h"
#include "tridot/components/RenderComponent.h"
#include "tridot/entity/Prefab.h"
#include <GLFW/glfw3.h>
#include <glm/gtx/vector_angle.hpp>
#include <imgui/imgui.h>
#include <algorithm>

using namespace tridot;

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

void lightGui(){
    if(ImGui::BeginTabItem("Lights")) {
        engine.view<Light, Transform>().each([](EntityId id, Light &light, Transform &transform) {
            ImGui::Separator();
            ImGui::PushID(id);

            std::vector<const char *> list = {"Ambient", "Directional", "Point Light"};
            ImGui::Combo("type", (int *) &light.type, list.data(), list.size());

            if(light.type == DIRECTIONAL_LIGHT){
                ImGui::DragFloat3("rotation", (float *) &transform.rotation, 0.01);
            }else{
                ImGui::DragFloat3("position", (float *) &transform.position, 0.01);
            }
            ImGui::ColorEdit3("color", (float *) &light.color);
            ImGui::DragFloat("intensity", &light.intensity, 0.01, 0.0, 1000);
            if (ImGui::Button("remove")) {
                engine.destroy(id);
            }

            ImGui::PopID();
        });
        ImGui::Separator();

        if (ImGui::Button("add light")) {
            engine.create(Transform(), Light(POINT_LIGHT, glm::vec3(Color::white.vec()), 1));
        }

        ImGui::EndTabItem();
    }
}

void materialGui(){
    if(ImGui::BeginTabItem("Materials")) {
        std::map<Material *, bool> materials;
        engine.view<RenderComponent>().each([&](RenderComponent &rc) {
            if (rc.material.get() != nullptr) {
                materials[rc.material.get()] = true;
            }
        });

        for (auto &m : materials) {
            ImGui::Separator();
            ImGui::PushID((int) (size_t) m.first);

            Material *material = m.first;

            std::vector<const char*> list = {"UV", "tri-planar", "scaled tri-planar"};
            ImGui::Combo("mapping", (int*)&material->mapping, list.data(), list.size());



            glm::vec4 c = material->color.vec();
            ImGui::ColorEdit3("color", (float *) &c);
            material->color = Color(c);

            ImGui::DragFloat("roughness", &material->roughness, 0.001, 0.0, 1.0);
            ImGui::DragFloat("metalic", &material->metallic, 0.001, 0.0, 1.0);

            ImGui::PopID();
        }

        ImGui::EndTabItem();
    }
}

int main(int argc, char *argv[]){
    Log::options.logLevel = Log::TRACE;
    engine.init(1920, 1080, "Tridot " TRI_VERSION, "../res/", true);
    engine.window.setBackgroundColor(Color::white);

    engine.resources.setup<Mesh>("cube")
            .setPostLoad([](Ref<Mesh> &mesh){MeshFactory::createCube(mesh); return true;})
            .setPreLoad(nullptr);
    engine.resources.setup<Mesh>("sphere")
            .setPostLoad([](Ref<Mesh> &mesh){MeshFactory::createSphere(32, 32, mesh); return true;})
            .setPreLoad(nullptr);
    createScene();

    //create lights
    engine.create(
            Transform(),
            Light(AMBIENT_LIGHT, glm::vec3(Color::white.vec()), 0.6));
    engine.create(
            Transform({0, 0, 0}, {1, 1, 1}, glm::radians(glm::vec3(80, 35, -145))),
            Light(DIRECTIONAL_LIGHT, glm::vec3(Color::white.vec()), 2.5));

    //create player
    Ref<Material> material = material.make();
    material->textureScale = {0.5f, 0.5f};
    material->mapping = Material::TRI_PLANAR;
    material->roughness = 2;
    material->metallic = 2;
    material->normalMap = engine.resources.get<Texture>("textures/Metal038_1K_Normal.jpg");
    material->texture = engine.resources.get<Texture>("textures/Metal038_1K_Color.jpg");
    material->roughnessMap = engine.resources.get<Texture>("textures/Metal038_1K_Roughness.jpg");
    material->metallicMap = engine.resources.get<Texture>("textures/Metal038_1K_Metalness.jpg");
    material->normalMap->setMagMin(false, false);
    material->roughnessMapScale = {0.5f, 0.5f};
    material->metallicMapScale = {0.5f, 0.5f};
    material->normalMapFactor = 1.0;

    *engine.resources.get<Prefab>("player") = Prefab(
        Transform(),
        RenderComponent()
            .setMaterial(material)
            .setMesh(engine.resources.get<Mesh>("sphere")),
        RigidBody(5),
        Collider(Collider::SPHERE)
    );
    EntityId playerId = engine.resources.get<Prefab>("player")->instantiate(engine);

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


        static bool debugOpen = false;
        if(engine.input.pressed('P')){
            debugOpen = !debugOpen;
        }
        if(debugOpen && ImGui::GetCurrentContext() != nullptr){
            ImGui::Begin("Debug Menu", &debugOpen);
            ImGui::BeginTabBar("tabs");
            lightGui();
            materialGui();
            ImGui::EndTabBar();
            ImGui::End();
        }


        engine.view<PerspectiveCamera>().each([&](PerspectiveCamera &camera){
            playerControl(playerId, camera);
        });
    }, "camera");

    engine.run();
    return 0;
}

void playerControl(EntityId playerId, PerspectiveCamera &camera){
    float speed = 20;
    static float jumpBufferTimer = 0;
    static float cameraDistance = 5;

    if(!engine.exists(playerId)){
        return;
    }
    if(!engine.hasAll<RigidBody, Transform, Collider>(playerId)){
        return;
    }

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
    if(look){
        cameraDistance /= std::pow(1.3f, engine.input.getMouseWheelDelta());
    }
    camera.position = transform.position - camera.forward * cameraDistance;
}

void createScene(){
    Ref<Mesh> cube = engine.resources.get<Mesh>("cube");

    int groundSize = 200;
    int wallHeight = 30;

    Ref<Material> wallMaterial = Ref<Material>::make();
    wallMaterial->mapping = Material::TRI_PLANAR;
    wallMaterial->roughness = 2;
    wallMaterial->metallic = 0;
    wallMaterial->normalMapFactor = 1.0;
    wallMaterial->normalMap = engine.resources.get<Texture>("textures/Tiles090_1K_Normal.jpg");
    wallMaterial->texture = engine.resources.get<Texture>("textures/Tiles090_1K_Color.jpg");
    wallMaterial->roughnessMap = engine.resources.get<Texture>("textures/Tiles090_1K_Roughness.jpg");
    wallMaterial->texture->setMagMin(false, false);
    wallMaterial->mapping = Material::SCALE_TRI_PLANAR;

    Prefab wall(
            Transform(),
            RenderComponent(Color::white * 0.7f).setMesh(cube).setMaterial(wallMaterial),
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

    Ref<Material> platformMaterial = Ref<Material>::make();
    platformMaterial->mapping = Material::TRI_PLANAR;
    platformMaterial->roughness = 2;
    platformMaterial->metallic = 0;
    platformMaterial->normalMapFactor = 1.0;
    platformMaterial->normalMap = engine.resources.get<Texture>("textures/Marble012_1K_Normal.jpg");
    platformMaterial->texture = engine.resources.get<Texture>("textures/Marble012_1K_Color.jpg");
    platformMaterial->roughnessMap = engine.resources.get<Texture>("textures/Marble012_1K_Roughness.jpg");
    platformMaterial->texture->setMagMin(false, false);
    platformMaterial->mapping = Material::SCALE_TRI_PLANAR;

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
                        RenderComponent(randu3() * 0.05f + 0.5f).setMesh(cube).setMaterial(platformMaterial),
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

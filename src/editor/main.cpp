//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/engine/Engine.h"
#include "tridot/engine/Serializer.h"
#include "tridot/render/Camera.h"
#include "tridot/render/MeshFactory.h"
#include "tridot/components/RenderComponent.h"
#include "Editor.h"
#include <imgui.h>

using namespace tridot;

int main(int argc, char *argv[]){
    Log::options.logLevel = Log::TRACE;

    engine.resources.addSearchDirectory(".");
    engine.init(1920, 1080, "Tridot Editor", "../res/", true);
    engine.window.setBackgroundColor(glm::vec4(glm::vec3((Color::white * 0.25).vec()), 1.0f));

    engine.resources.set<Mesh>("cube") = MeshFactory::createCube();
    engine.resources.set<Mesh>("sphere") = MeshFactory::createSphere(32, 32);
    engine.resources.get<Mesh>("teapot.obj");

    engine.resources.get<Texture>("checkerboard.png");
    engine.resources.get<Texture>("normal1.png");
    engine.resources.get<Texture>("normal2.png");
    engine.resources.get<Texture>("tex1.png");
    engine.resources.get<Texture>("tex2.png");
    engine.resources.get<Texture>("tex3.png");
    engine.resources.get<Texture>("tex4.png");

    engine.registerComponent<Transform, RenderComponent, PerspectiveCamera, OrthographicCamera, Light, RigidBody, Collider>();

    Editor::currentSceneFile = "scene.yaml";
    Serializer s;
    if(!s.load(Editor::currentSceneFile, engine, engine.resources)){
        Ref<Material> gold = engine.resources.set<Material>("gold");
        gold->texture = nullptr;
        gold->textureScale = {0.5f, 0.5f};
        gold->mapping = Material::SCALE_TRI_PLANAR;
        gold->roughness = 0.50;
        gold->metallic = 0.6;
        gold->normalMap = engine.resources.get<Texture>("normal2.png");
        gold->normalMap->setMagMin(false, false);
        gold->normalMapScale = {0.5f, 0.5f};
        gold->normalMapFactor = 0.5;
        gold->color = Color(194, 172, 84);
        gold->roughness = 0.651;
        gold->metallic = 1.0;

        Editor::cameraId = engine.create();
        PerspectiveCamera &camera = engine.add<PerspectiveCamera>(Editor::cameraId);
        camera.position = glm::vec3(3, 5, 2) * 0.5f;
        camera.forward = -glm::normalize(camera.position);
        camera.up = {0, 0, 1};
        camera.right = {1, 0, 0};
        camera.near = 0.05;
        camera.far = 1200;
        camera.aspectRatio = 1;

        engine.create(Transform(), RenderComponent(Color::white).setMesh(engine.resources.get<Mesh>("cube")).setMaterial(gold));
        engine.create(Light(DIRECTIONAL_LIGHT, {-0.3, 0.1, -0.4}, {1, 1, 1}, 2.7));
        engine.create(Light(AMBIENT_LIGHT, {0, 0, 0}, {1, 1, 1}, 0.5));
    }

    engine.onUpdate().add([&](){
        if(ImGui::GetCurrentContext() != nullptr){
            bool &open = Editor::getFlag("ImGui Demo");
            if(open){
                ImGui::ShowDemoWindow(&open);
            }
        }
    }, "panels");

    Editor::loadFlags();
    engine.run();
    Editor::saveFlags();
    return 0;
}

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/engine/Engine.h"
#include "tridot/render/Camera.h"
#include "tridot/render/MeshFactory.h"
#include "tridot/components/RenderComponent.h"
#include "tridot/components/Tag.h"
#include "Editor.h"
#include <imgui.h>

using namespace tridot;

void createDefaultScene();

int main(int argc, char *argv[]){
    Log::options.logLevel = Log::TRACE;
#if WIN32
    Log::options.colorEnabled = false;
#endif

    engine.resources.addSearchDirectory(".");
    engine.init(1920, 1080, "Tridot Editor", "../res/", true);
    engine.window.setBackgroundColor(glm::vec4(glm::vec3((Color::white * 0.25).vec()), 1.0f));
    ImGui::GetIO().IniFilename = "editor.ini";

    engine.resources.set<Mesh>("cube") = MeshFactory::createCube();
    engine.resources.set<Mesh>("sphere") = MeshFactory::createSphere(32, 32);
    engine.resources.get<Mesh>("teapot.obj", true);

    engine.resources.get<Texture>("checkerboard.png");
    engine.resources.get<Texture>("normal1.png");
    engine.resources.get<Texture>("normal2.png");
    engine.resources.get<Texture>("tex1.png");
    engine.resources.get<Texture>("tex2.png");
    engine.resources.get<Texture>("tex3.png");
    engine.resources.get<Texture>("tex4.png");

    Editor::currentSceneFile = "scene.yaml";
    if(!engine.loadScene(Editor::currentSceneFile)){
        createDefaultScene();
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
    engine.shutdown();
    return 0;
}

void createDefaultScene(){
    Ref<Material> gold = engine.resources.set<Material>("gold");
    gold->texture = nullptr;
    gold->textureScale = {0.5f, 0.5f};
    gold->mapping = Material::SCALE_TRI_PLANAR;
    gold->roughness = 0.65;
    gold->metallic = 1.0;
    gold->normalMap = engine.resources.get<Texture>("normal2.png");
    gold->normalMap->setMagMin(false, false);
    gold->normalMapScale = {0.7f, 0.7f};
    gold->normalMapFactor = 0.4;
    gold->color = Color(169, 146, 55);

    Editor::cameraId = engine.create(Tag("Camera"));
    PerspectiveCamera &camera = engine.add<PerspectiveCamera>(Editor::cameraId);
    camera.position = glm::vec3(3, 5, 2) * 0.5f;
    camera.forward = -glm::normalize(camera.position);
    camera.up = {0, 0, 1};
    camera.right = {1, 0, 0};
    camera.near = 0.05;
    camera.far = 1200;
    camera.aspectRatio = 1;

    engine.create(Tag("Cube"), Transform(), RenderComponent(Color::white).setMesh(engine.resources.get<Mesh>("cube")).setMaterial(gold));
    engine.create(Tag("Directional Light"), Light(DIRECTIONAL_LIGHT, {1, 1, 1}, 2.7),
                  Transform({0, 0, 0}, {1, 1, 1}, glm::radians(glm::vec3(80, 35, -145))));
    engine.create(Tag("Ambient Light"), Light(AMBIENT_LIGHT, {1, 1, 1}, 0.5), Transform());
}
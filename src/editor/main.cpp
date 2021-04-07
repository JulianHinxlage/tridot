//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/engine/Engine.h"
#include "tridot/render/Camera.h"
#include "tridot/render/MeshFactory.h"
#include "tridot/components/RenderComponent.h"
#include "Editor.h"
#include <imgui.h>

using namespace tridot;

int main(int argc, char *argv[]){
    Log::options.logLevel = Log::TRACE;
    engine.init(1920, 1080, "Tridot Editor", "../res/", true);
    engine.window.setBackgroundColor(Color::white * 0.5);

    engine.resources.set<Mesh>("cube") = MeshFactory::createCube();
    engine.resources.set<Mesh>("sphere") = MeshFactory::createSphere(32, 32);
    engine.resources.get<Mesh>("teapot.obj");

    Ref<Material> gold = engine.resources.set<Material>("gold");
    gold->texture = engine.resources.get<Texture>("tex1.png");
    gold->textureScale = {0.5f, 0.5f};
    gold->mapping = Material::SCALE_TRI_PLANAR;
    gold->roughness = 0.50;
    gold->metallic = 0.6;
    gold->normalMap = engine.resources.get<Texture>("normal2.png");
    gold->normalMap->setMagMin(false, false);
    gold->normalMapScale = {0.5f, 0.5f};
    gold->normalMapFactor = 0.5;
    gold->color = Color(243, 217,105) * 0.8f;
    gold->roughness = 0.651;
    gold->metallic = 1.0;
    gold->texture = nullptr;

    editor.cameraId = engine.create();
    PerspectiveCamera &camera = engine.add<PerspectiveCamera>(editor.cameraId);
    camera.position = glm::vec3(3, -5, 2) * 0.5f;
    camera.forward = -glm::normalize(camera.position);
    camera.up = {0, 0, 1};
    camera.right = {1, 0, 0};
    camera.near = 0.05;
    camera.far = 1200;
    camera.aspectRatio = 1;

    engine.create(Transform(), RenderComponent(Color::white).setMesh(engine.resources.get<Mesh>("cube")).setMaterial(gold));
    engine.create(Light(DIRECTIONAL_LIGHT, {-0.3, 0.1, -0.4}, {1, 1, 1}, 4.0));
    engine.create(Light(AMBIENT_LIGHT, {0, 0, 0}, {1, 1, 1}, 0.5));

    editor.loadFlags();

    engine.onUpdate().add([&](){
        if(ImGui::GetCurrentContext() != nullptr){
            bool &open = editor.getFlag("ImGui Demo");
            if(open){
                ImGui::ShowDemoWindow(&open);
            }
        }
    }, "panels");

    engine.run();
    editor.saveFlags();
}

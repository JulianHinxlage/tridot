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
    Log::options.logLevel = Log::DEBUG;
#if WIN32
    Log::options.colorEnabled = false;
#endif

    engine.resources.addSearchDirectory(".");
    engine.init(1920, 1080, "Tridot Editor", "../res/", true);
    engine.window.setBackgroundColor(glm::vec4( 0.25, 0.25, 0.25, 1 ));
    Editor::init();

    engine.resources.set<Mesh>("cube") = MeshFactory::createCube();
    engine.resources.set<Mesh>("sphere") = MeshFactory::createSphere(32, 32);
    engine.resources.get<Mesh>("teapot.obj", true);

    engine.resources.get<Texture>("Checkerboard.png");

    engine.resources.get<Texture>("Tiles090_1K_Color.jpg");
    engine.resources.get<Texture>("Tiles090_1K_Normal.jpg");
    engine.resources.get<Texture>("Tiles090_1K_Roughness.jpg");

    engine.resources.get<Texture>("Wood049_1K_Color.jpg");
    engine.resources.get<Texture>("Wood049_1K_Normal.jpg");
    engine.resources.get<Texture>("Wood049_1K_Roughness.jpg");

    engine.resources.get<Texture>("Metal038_1K_Color.jpg");
    engine.resources.get<Texture>("Metal038_1K_Normal.jpg");
    engine.resources.get<Texture>("Metal038_1K_Roughness.jpg");
    engine.resources.get<Texture>("Metal038_1K_Metalness.jpg");

    engine.resources.get<Texture>("Marble012_1K_Color.jpg");
    engine.resources.get<Texture>("Marble012_1K_Normal.jpg");
    engine.resources.get<Texture>("Marble012_1K_Roughness.jpg");

    engine.resources.get<Texture>("Ground037_1K_Color.jpg");
    engine.resources.get<Texture>("Ground037_1K_Normal.jpg");
    engine.resources.get<Texture>("Ground037_1K_Roughness.jpg");

    engine.resources.get<Texture>("Rocks022_1K_Color.jpg");
    engine.resources.get<Texture>("Rocks022_1K_Normal.jpg");
    engine.resources.get<Texture>("Rocks022_1K_Roughness.jpg");

    Editor::currentSceneFile = "../res/scenes/scene.yml";
    if(!engine.loadScene(Editor::currentSceneFile)){
        createDefaultScene();
    }

    engine.onUpdate().add([&](){
        if(ImGui::GetCurrentContext() != nullptr){
            {
                bool &open = Editor::getFlag("ImGui Demo");
                if (open) {
                    ImGui::ShowDemoWindow(&open);
                }
            }
            {
                bool &open = Editor::getFlag("Statistics");
                static std::vector<float> frameTimes;
                frameTimes.push_back(engine.time.frameTime);
                while(frameTimes.size() > 305){
                    frameTimes.erase(frameTimes.begin());
                }
                if (open) {
                    if(ImGui::Begin("Statistics", &open)){
                        ImGui::Text("fps: %f", engine.time.framesPerSecond);
                        ImGui::PlotLines("frame time", frameTimes.data() + 5, (int)frameTimes.size() - 5, 0,
                            (std::to_string(engine.time.frameTime * 1000.0f) + "ms").c_str(), std::numeric_limits<float>::max(),
                            std::numeric_limits<float>::max(), ImVec2(300, 100));

                        bool vsync = engine.window.getVSync();
                        if(ImGui::Checkbox("VSync", &vsync)){
                            engine.window.setVSync(vsync);
                        }
                        ImGui::Text("%i entities selected", (int)Editor::selection.selectedEntities.size());
                    }
                    ImGui::End();
                }
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
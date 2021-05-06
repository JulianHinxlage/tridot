//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/engine/Engine.h"
#include "tridot/render/Camera.h"
#include "tridot/render/MeshFactory.h"
#include "tridot/components/RenderComponent.h"
#include "tridot/components/Tag.h"
#include "tridot/engine/Plugin.h"
#include "Editor.h"
#include <imgui.h>

using namespace tridot;

void createDefaultScene(Scene &scene);

int main(int argc, char *argv[]){
    Log::options.logLevel = Log::DEBUG;
#if WIN32
    Log::options.colorEnabled = false;
#endif

    engine.resources.addSearchDirectory(".");
    engine.resources.threadCount = 4;
    engine.init(1920, 1080, "Tridot Editor", "../res/", true);
    engine.window.setBackgroundColor(glm::vec4( 0.25, 0.25, 0.25, 1 ));
    Editor::init();
    engine.resources.update();

    engine.resources.setup<Mesh>("cube")
            .setPostLoad([](Ref<Mesh> &mesh){MeshFactory::createCube(mesh); return true;})
            .setPreLoad(nullptr);
    engine.resources.setup<Mesh>("sphere")
            .setPostLoad([](Ref<Mesh> &mesh){MeshFactory::createSphere(32, 32, mesh); return true;})
            .setPreLoad(nullptr);
    engine.resources.setup<Mesh>("teapot.obj");

    engine.resources.setup<Texture>("Checkerboard.png");

    engine.resources.setup<Texture>("Tiles090_1K_Color.jpg");
    engine.resources.setup<Texture>("Tiles090_1K_Normal.jpg");
    engine.resources.setup<Texture>("Tiles090_1K_Roughness.jpg");

    engine.resources.setup<Texture>("Wood049_1K_Color.jpg");
    engine.resources.setup<Texture>("Wood049_1K_Normal.jpg");
    engine.resources.setup<Texture>("Wood049_1K_Roughness.jpg");

    engine.resources.setup<Texture>("Metal038_1K_Color.jpg");
    engine.resources.setup<Texture>("Metal038_1K_Normal.jpg");
    engine.resources.setup<Texture>("Metal038_1K_Roughness.jpg");
    engine.resources.setup<Texture>("Metal038_1K_Metalness.jpg");

    engine.resources.setup<Texture>("Marble012_1K_Color.jpg");
    engine.resources.setup<Texture>("Marble012_1K_Normal.jpg");
    engine.resources.setup<Texture>("Marble012_1K_Roughness.jpg");

    engine.resources.setup<Texture>("Ground037_1K_Color.jpg");
    engine.resources.setup<Texture>("Ground037_1K_Normal.jpg");
    engine.resources.setup<Texture>("Ground037_1K_Roughness.jpg");

    engine.resources.setup<Texture>("Rocks022_1K_Color.jpg");
    engine.resources.setup<Texture>("Rocks022_1K_Normal.jpg");
    engine.resources.setup<Texture>("Rocks022_1K_Roughness.jpg");

    Editor::currentSceneFile = "scene.yml";
    if(engine.resources.searchFile(Editor::currentSceneFile) != ""){
        Editor::currentSceneFile = engine.resources.searchFile(Editor::currentSceneFile);
        engine.resources.setup<Scene>(Editor::currentSceneFile).setInstance(&engine).get();
    }else{
        engine.resources.setup<Scene>("scene")
                .setInstance(&engine)
                .setOptions(ResourceManager::LOAD_WITHOUT_FILE)
                .setPreLoad([](Ref<Scene> &scene, const std::string &file){
                    createDefaultScene(*scene);
                    return true;
                }).get();
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
                        ImGui::Text("%i entities in scene", (int)engine.getEntityPool().getEntities().size());
                        if(Editor::selection.selectedEntities.size() == 1){
                            ImGui::Text("selected entity id: %i", (int)Editor::selection.selectedEntities.begin()->first);
                        }
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

void createDefaultScene(Scene &scene){
    Editor::cameraId = scene.create(Tag("Camera"));
    PerspectiveCamera &camera = scene.add<PerspectiveCamera>(Editor::cameraId);
    camera.position = glm::vec3(3, 5, 2) * 0.5f;
    camera.forward = -glm::normalize(camera.position);
    camera.up = {0, 0, 1};
    camera.right = {1, 0, 0};
    camera.near = 0.05;
    camera.far = 1200;
    camera.aspectRatio = 1;

    scene.create(Tag("Cube"), Transform(), RenderComponent(Color::white).setMesh(engine.resources.get<Mesh>("cube")));
    scene.create(Tag("Directional Light"), Light(DIRECTIONAL_LIGHT, {1, 1, 1}, 2.7),
                  Transform({0, 0, 0}, {1, 1, 1}, glm::radians(glm::vec3(80, 35, -145))));
    scene.create(Tag("Ambient Light"), Light(AMBIENT_LIGHT, {1, 1, 1}, 0.5), Transform());
}
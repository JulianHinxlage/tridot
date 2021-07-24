//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/engine/Time.h"
#include "tridot/render/Window.h"
#include "tridot/render/Camera.h"
#include "tridot/render/Light.h"
#include "tridot/render/MeshFactory.h"
#include "tridot/components/RenderComponent.h"
#include "tridot/components/Tag.h"
#include "Editor.h"
#include <imgui.h>

using namespace tridot;

void createDefaultScene(Scene &scene);

int main(int argc, char *argv[]){
    env->console->options.level = DEBUG;
    env->console->options.date = false;
    env->console->addLogFile("log.txt", Console::Options(TRACE, true, true, false));
    env->console->addLogFile("error.txt", Console::Options(ERROR, true, true, false));
#if WIN32
    env->console->options.color = false;
#endif

    env->events->init.invoke();
    env->events->init.setActiveAll(false);
    env->editor->init();
    env->console->info("Tridot version: ", TRI_VERSION);

    bool running = true;
    env->events->exit.addCallback([&running](){
        running = false;
    });

    env->resources->defaultOptions<Scene>().setPostLoad([](Ref<Scene> &scene){
        bool valid = scene->postLoad();
        env->scene->swap(*scene);
        env->resources->remove(env->resources->getName(scene));
        return valid;
    });

    std::string sceneFile = "scenes/scene.yml";
    if(env->resources->searchFile(sceneFile) != ""){
        sceneFile = env->resources->searchFile(sceneFile);
        env->resources->setup<Scene>(sceneFile).setInstance(env->scene).get();
    }else{
        env->scene->name = sceneFile;
        env->resources->setup<Scene>("scene")
            .setOptions(ResourceManager::LOAD_WITHOUT_FILE)
            .setPreLoad([](Ref<Scene> &scene, const std::string &file){
                createDefaultScene(*scene);
                return true;
            }).get();
    }

    env->events->update.addCallback([&](){
        if(ImGui::GetCurrentContext() != nullptr){
            {
                bool imGuiDemoEnabled = false;
                if(imGuiDemoEnabled) {
                    bool &open = env->editor->getFlag("ImGui Demo");
                    if (open) {
                        ImGui::ShowDemoWindow(&open);
                    }
                }
            }
            {
                bool &open = env->editor->getFlag("Statistics");
                static std::vector<float> frameTimes;
                frameTimes.push_back(env->time->frameTime);
                while(frameTimes.size() > 305){
                    frameTimes.erase(frameTimes.begin());
                }
                if (open) {
                    if(ImGui::Begin("Statistics", &open)){
                        ImGui::Text("fps: %f", env->time->framesPerSecond);
                        ImGui::PlotLines("frame time", frameTimes.data() + 5, (int)frameTimes.size() - 5, 0,
                            (std::to_string(env->time->frameTime * 1000.0f) + "ms").c_str(), std::numeric_limits<float>::max(),
                            std::numeric_limits<float>::max(), ImVec2(300, 100));

                        bool vsync = env->window->getVSync();
                        if(ImGui::Checkbox("VSync", &vsync)){
                            env->window->setVSync(vsync);
                        }
                        ImGui::Text("%i entities selected", (int)env->editor->selection.entities.size());
                        ImGui::Text("%i entities in scene", (int)env->scene->getEntityPool().getEntities().size());
                        if(env->editor->selection.entities.size() == 1){
                            ImGui::Text("selected entity id: %i", (int)env->editor->selection.entities.begin()->first);
                        }
                    }
                    ImGui::End();
                }
            }
        }
    }, "panels");

    env->editor->loadFlags();
    while(running){
        env->events->init.invoke();
        env->events->init.setActiveAll(false);
        env->events->update.invoke();
        env->events->pollEvents();
    }
    env->editor->saveFlags();
    env->events->shutdown.invoke();
    return 0;
}

void createDefaultScene(Scene &scene){
    env->editor->cameraId = scene.create(Tag("Camera"));
    PerspectiveCamera &camera = scene.add<PerspectiveCamera>(env->editor->cameraId);
    Transform &transform = scene.add<Transform>(env->editor->cameraId);
    transform.position = glm::vec3(3, 5, 2) * 0.5f;
    transform.rotation.x = glm::radians(60.0);
    transform.rotation.z = glm::radians(160.0);
    camera.forward = -glm::normalize(transform.position);
    camera.up = {0, 0, 1};
    camera.right = {1, 0, 0};
    camera.near = 0.05;
    camera.far = 1200;
    camera.aspectRatio = 1;

    scene.create(Tag("Cube"), Transform(), RenderComponent(Color::white).setMesh(env->resources->get<Mesh>("cube")));
    scene.create(Tag("Directional Light"), Light(DIRECTIONAL_LIGHT, {1, 1, 1}, 2.7),
                  Transform({0, 0, 0}, {1, 1, 1}, glm::radians(glm::vec3(80, 35, -145))));
    scene.create(Tag("Ambient Light"), Light(AMBIENT_LIGHT, {1, 1, 1}, 0.5), Transform());
}
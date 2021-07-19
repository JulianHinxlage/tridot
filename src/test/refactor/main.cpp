//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/core/Environment.h"
#include "tridot/engine/engine.h"
#include "tridot/render/Window.h"

#include "tridot/render/Camera.h"
#include "tridot/render/Shader.h"
#include "tridot/render/MeshRenderer.h"

using namespace tridot;

TRI_UPDATE_CALLBACK("clear") {
    env->scene->view<PerspectiveCamera>().each([](PerspectiveCamera &camera) {
        if (camera.target.get() == nullptr) {
            camera.target = Ref<FrameBuffer>::make();
            camera.target->init(env->window->getSize().x, env->window->getSize().y,
                                {{COLOR, env->window->getBackgroundColor()},
                                 {DEPTH}});
        }
        if (camera.target->getSize() != env->window->getSize()) {
            camera.target->resize(env->window->getSize().x, env->window->getSize().y);
        }
        camera.aspectRatio = env->window->getAspectRatio();
        camera.target->clear();
    });
}
TRI_UPDATE_CALLBACK("draw"){
    bool drawn = false;
    env->scene->view<PerspectiveCamera>().each([&](PerspectiveCamera &camera){
        if(!drawn && camera.output.get() != nullptr){
            auto shader = env->resources->get<Shader>("shaders/mesh.glsl");
            shader->bind();
            shader->set("uTriPlanarMapping", false);
            env->renderer->begin(glm::mat4(1), {0, 0, 0}, nullptr);
            env->renderer->submit({{0, 0, 0}, {2, 2, 2}}, camera.output->getAttachment(COLOR).get());
            env->renderer->end();
        }
        drawn = true;
    });
}

int main(int argc, char *argv[]) {
    //set logging options
    env->console->options.level = DEBUG;
    env->console->addLogFile("log.txt", Console::Options(TRACE, true, true, false));
    env->console->addLogFile("error.txt", Console::Options(ERROR, true, true, false));

    //init
    env->events->init.invoke();

    bool running = true;
    env->events->exit.addCallback([&running](){
        running = false;
    });

    //load scene
    env->resources->setup<Scene>("scenes/scene.yml").setInstance(env->scene).get();

    //wait for scene to load but still update window
    while(env->resources->isLoading()){
        env->window->update();
        env->resources->update();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    //main loop
    while(running){
        env->events->update.invoke();
        env->events->pollEvents();
    }

    //shutdown
    env->events->shutdown.invoke();
}


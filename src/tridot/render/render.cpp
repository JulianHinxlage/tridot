//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "render.h"
#include "tridot/engine/ResourceManager.h"
#include "tridot/engine/Scene.h"
#include "tridot/engine/Input.h"

void imguiInit();

namespace tridot{

    TRI_REGISTER_CALLBACK(){
        env->events->init.callbackOrder({"engine", "render"});
    }

    TRI_INIT_CALLBACK("render"){
        env->window = env->systems->addSystem<Window>();
        env->renderer = env->systems->addSystem<MeshRenderer>();
        env->pbRenderer = env->systems->addSystem<PBRenderer>();

        env->resources->setup<Mesh>("cube")
                .setPostLoad([](Ref<Mesh> &mesh){MeshFactory::createCube(mesh); return true;})
                .setPreLoad(nullptr);
        env->resources->setup<Mesh>("sphere")
                .setPostLoad([](Ref<Mesh> &mesh){MeshFactory::createSphere(32, 32, mesh); return true;})
                .setPreLoad(nullptr);
        env->resources->setup<Mesh>("quad")
                .setPostLoad([](Ref<Mesh> &mesh){MeshFactory::createQuad(mesh); return true;})
                .setPreLoad(nullptr);
        env->resources->setup<Mesh>("circle")
                .setPostLoad([](Ref<Mesh> &mesh){MeshFactory::createRegularPolygon(64, mesh); return true;})
                .setPreLoad(nullptr);

        env->window->init(1920, 1080, "Tridot Game Engine");
        env->window->setBackgroundColor(glm::vec4( 0.25, 0.25, 0.25, 1 ));
        env->renderer->init(env->resources->get<Shader>("shaders/mesh.glsl"));
        env->pbRenderer->init(env->resources->get<Shader>("shaders/PBR.glsl"));
        env->input->init();

        RenderContext::setDepth(true);
        imguiInit();

        env->events->update.callbackOrder({"imgui end", "window", "imgui begin", "resources", "clear", "skybox", "rendering", "post processing", "transform", "physics"});
        env->events->update.callbackOrder({"clear", "rendering", "post processing", "draw"});
    }

    /*
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
     */

    TRI_UPDATE_CALLBACK("window"){
        env->window->update();
        if(!env->window->isOpen()){
            env->events->exit.invoke();
        }
    }

}
//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/engine/Engine.h"
#include "tridot/render/Camera.h"

using namespace tridot;

int main(int argc, char *argv[]){
    engine.resources.addSearchDirectory("plugins/");
    engine.init(1920, 1080, "Tridot Launcher", "../res/", false);
    engine.window.setBackgroundColor(glm::vec4( 0.25, 0.25, 0.25, 1 ));

    std::string sceneFile = "scenes/scene.yml";
    if(engine.resources.searchFile(sceneFile) != ""){
        sceneFile = engine.resources.searchFile(sceneFile);
        engine.resources.get<Scene>(sceneFile);
        while(engine.resources.isLoading()){
            engine.window.update();
            engine.resources.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    engine.onUpdate().add([](){
       engine.view<PerspectiveCamera>().each([](PerspectiveCamera &camera){
            if(camera.target.get() == nullptr){
                camera.target = Ref<FrameBuffer>::make();
                camera.target->init(engine.window.getSize().x, engine.window.getSize().y, {{COLOR, engine.window.getBackgroundColor()}, {DEPTH}});
            }
            if(camera.target->getSize() != engine.window.getSize()){
                camera.target->resize(engine.window.getSize().x, engine.window.getSize().y);
            }
            camera.target->clear();
       });
    }, "clear");

    engine.onUpdate().add([](){
        bool drawn = false;
        engine.view<PerspectiveCamera>().each([&](PerspectiveCamera &camera){
            if(!drawn && camera.output.get() != nullptr){
                auto shader = engine.resources.get<Shader>("shaders/mesh.glsl");
                shader->bind();
                shader->set("uTriPlanarMapping", false);
                engine.renderer.begin(glm::mat4(1), {0, 0, 0}, nullptr);
                engine.renderer.submit({{0, 0, 0}, {2, 2, 2}}, camera.output->getAttachment(COLOR).get());
                engine.renderer.end();
            }
            drawn = true;
        });
    }, "draw");

    engine.onUpdate().order({"clear", "rendering", "post processing", "draw"});

    engine.run();
    engine.shutdown();
    return 0;
}

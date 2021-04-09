//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "EditorCamera.h"
#include "tridot/engine/Engine.h"

namespace tridot {

    void EditorCamera::update(PerspectiveCamera &camera, bool hovered) {
        glm::vec2 mousePosition = engine.input.getMousePosition(false);

        if(hovered && engine.input.pressed(Input::MOUSE_BUTTON_MIDDLE)){
            startMousePosition = mousePosition;
            dragMiddle = true;
        }
        if(hovered && engine.input.pressed(Input::MOUSE_BUTTON_RIGHT)){
            startMousePosition = mousePosition;
            dragRight = true;
        }

        glm::vec3 up = glm::cross(camera.forward, camera.right);

        if(dragMiddle && engine.input.down(Input::MOUSE_BUTTON_MIDDLE)){
            camera.position -= (mousePosition - startMousePosition).x * camera.right * speed * 0.005f;
            camera.position -= (mousePosition - startMousePosition).y * up * speed * 0.005f;
        }
        if(dragRight && engine.input.down(Input::MOUSE_BUTTON_RIGHT)){
            camera.forward += (mousePosition - startMousePosition).x * camera.right * 0.001f;
            camera.forward += (mousePosition - startMousePosition).y * up * 0.001f;
        }
        if(hovered){
            camera.position += camera.forward * speed * 0.5f * engine.input.getMouseWheelDelta();
        }

        if(engine.input.released(Input::MOUSE_BUTTON_MIDDLE)){
            dragMiddle = false;
        }
        if(engine.input.released(Input::MOUSE_BUTTON_RIGHT)){
            dragRight = false;
        }

        if(dragMiddle || dragRight){
            engine.input.setMousePosition(startMousePosition, false);
        }

        if(hovered){
            if(engine.input.down('W')){
                camera.position += camera.forward * engine.time.frameTime * speed * 5.0f;
            }if(engine.input.down('A')){
                camera.position -= camera.right * engine.time.frameTime * speed * 5.0f;
            }if(engine.input.down('S')){
                camera.position -= camera.forward * engine.time.frameTime * speed * 5.0f;
            }if(engine.input.down('D')){
                camera.position += camera.right * engine.time.frameTime * speed * 5.0f;
            }
        }
    }

}
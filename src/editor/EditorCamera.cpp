//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "EditorCamera.h"
#include "tridot/engine/Engine.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace tridot {

    void EditorCamera::update(PerspectiveCamera &camera, Transform &transform, bool hovered, bool useKeyboard) {
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
            transform.position += (mousePosition - startMousePosition).x * camera.right * speed * 0.005f;
            transform.position -= (mousePosition - startMousePosition).y * up * speed * 0.005f;
        }
        if(dragRight && engine.input.down(Input::MOUSE_BUTTON_RIGHT)){
            glm::vec2 move = (mousePosition - startMousePosition) * 0.001f;
            transform.rotation -= glm::vec3(move.y, 0, move.x);
            transform.rotation.x = std::max(glm::radians(0.01f), transform.rotation.x);
            transform.rotation.x = std::min(glm::radians(179.99f), transform.rotation.x);
        }
        if(hovered){
            transform.position -= camera.forward * speed * 0.5f * engine.input.getMouseWheelDelta();
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

        if(hovered && useKeyboard){
            if(engine.input.down('W')){
                transform.position -= camera.forward * engine.time.frameTime * speed * 5.0f;
            }if(engine.input.down('A')){
                transform.position += camera.right * engine.time.frameTime * speed * 5.0f;
            }if(engine.input.down('S')){
                transform.position += camera.forward * engine.time.frameTime * speed * 5.0f;
            }if(engine.input.down('D')){
                transform.position -= camera.right * engine.time.frameTime * speed * 5.0f;
            }
        }
    }

}
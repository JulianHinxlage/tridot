//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "EditorCamera.h"
#include "tridot/engine/Engine.h"
#include <glm/gtc/matrix_transform.hpp>

namespace tridot {

    void EditorCamera::update(PerspectiveCamera &camera, bool hovered, bool useKeyboard) {
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
            glm::vec2 move = (mousePosition - startMousePosition) * 0.001f;
            camera.forward = glm::vec3(glm::vec4(camera.forward, 1.0) * glm::rotate(glm::mat4(1), move.x, camera.up));
            float theta = glm::dot(camera.forward, camera.up);
            if(theta > 0.99 & move.y < 0){
                move.y = 0;
            }if(theta < -0.99 & move.y > 0){
                move.y = 0;
            }
            camera.forward = glm::vec3(glm::vec4(camera.forward, 1.0) * glm::rotate(glm::mat4(1), move.y, camera.right));
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

        if(hovered && useKeyboard){
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
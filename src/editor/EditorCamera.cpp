//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "EditorCamera.h"
#include "core/core.h"
#include "engine/Input.h"
#include "engine/Time.h"

namespace tri {

    void EditorCamera::update(Camera &camera, Transform &transform) {
        glm::vec3 move = {0, 0, 0};
        if(env->input->down("W")){
            move.z -= 1;
        }
        if(env->input->down("A")){
            move.x -= 1;
        }
        if(env->input->down("S")){
            move.z += 1;
        }
        if(env->input->down("D")){
            move.x += 1;
        }
        if(env->input->down("Q")){
            move.y -= 1;
        }
        if(env->input->down("E")){
            move.y += 1;
        }

        //WASD EQ
        if(camera.type == Camera::PERSPECTIVE) {
            move *= speed * env->time->deltaTime;
            move /= transform.scale;
            transform.position += camera.forward * -move.z;
            transform.position += camera.right * move.x;
            transform.position += camera.up * move.y;
        }else{
            move *= env->time->deltaTime * 1.0f;
            move.z *= transform.scale.y;
            move.x *= transform.scale.x;
            transform.position += camera.up * -move.z;
            transform.position += camera.right * move.x;
        }

        //mouse wheel
        float wheelDelta = env->input->getMouseWheelDelta();
        if(env->input->down(Input::KEY_LEFT_SHIFT) || env->input->down(Input::KEY_RIGHT_SHIFT)){
            speed *= std::pow(1.1, wheelDelta);
        }else{
            if(camera.type == Camera::PERSPECTIVE) {
                transform.position += wheelDelta * camera.forward * speed * 0.1f;
            }else{
                transform.scale /= std::pow(1.1, wheelDelta);
            }
        }

        glm::vec2 mousePosition = env->input->getMousePosition();
        if(env->input->pressed(Input::MOUSE_BUTTON_RIGHT)){
            startMousePosition = mousePosition;
        }
        if(env->input->pressed(Input::MOUSE_BUTTON_MIDDLE)){
            startMousePosition = mousePosition;
        }
        if(env->input->down(Input::MOUSE_BUTTON_RIGHT)){
            if(camera.type == Camera::PERSPECTIVE) {
                //lock around
                glm::vec2 delta = mousePosition - startMousePosition;
                transform.rotation.y -= delta.x * 0.001 / transform.scale.z;
                transform.rotation.x -= delta.y * 0.001 / transform.scale.z;
                transform.rotation.x = glm::radians( std::min(89.0f, std::max(-89.0f, glm::degrees(transform.rotation.x))));
                env->input->setMousePosition(startMousePosition);
            }
        }
        if(env->input->down(Input::MOUSE_BUTTON_MIDDLE)){
            //pan move
            glm::vec2 delta = mousePosition - startMousePosition;
            transform.position += delta.y * camera.up * 0.001f * speed * transform.scale.y;
            transform.position -= delta.x * camera.right * 0.001f * speed * transform.scale.x;
            env->input->setMousePosition(startMousePosition);
        }

    }

}

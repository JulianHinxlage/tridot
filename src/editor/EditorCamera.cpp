//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "EditorCamera.h"
#include "core/core.h"
#include "window/Input.h"
#include "engine/Time.h"
#include "entity/World.h"

namespace tri {

    void EditorCamera::update(Camera &camera, Transform &transform) {
        bool shift = env->input->down(Input::KEY_LEFT_SHIFT) || env->input->down(Input::KEY_RIGHT_SHIFT);
        bool control = env->input->down(Input::KEY_LEFT_CONTROL) || env->input->down(Input::KEY_RIGHT_CONTROL);

        glm::vec3 move = {0, 0, 0};
        if(!control) {
            if (env->input->down("W")) {
                move.z -= 1;
            }
            if (env->input->down("A")) {
                move.x -= 1;
            }
            if (env->input->down("S")) {
                move.z += 1;
            }
            if (env->input->down("D")) {
                move.x += 1;
            }
            if (env->input->down("Q")) {
                move.y -= 1;
            }
            if (env->input->down("E")) {
                move.y += 1;
            }
        }

        glm::vec3 position = transform.getWorldPosition();

        //WASD EQ
        if(camera.type == Camera::PERSPECTIVE) {
            move *= speed * env->time->frameTime;
            move /= transform.scale;
            position += camera.forward * -move.z;
            position += camera.right * move.x;
            position += camera.up * move.y;
        }else{
            move *= env->time->frameTime * 1.0f;
            move.z *= transform.scale.y;
            move.x *= transform.scale.x;
            position += camera.up * -move.z;
            position += camera.right * move.x;
        }

        //mouse wheel
        float wheelDelta = env->input->getMouseWheelDelta();
        if(shift){
            speed *= std::pow(1.2, wheelDelta);
        }else{
            if(camera.type == Camera::PERSPECTIVE) {
                position += wheelDelta * camera.forward * speed * 0.1f;
            }else{
                transform.scale /= std::pow(1.1, wheelDelta);
            }
        }

        glm::vec2 mousePosition = env->input->getMousePosition();
        if(env->input->pressed(Input::MOUSE_BUTTON_RIGHT)){
            startMousePosition = mousePosition;
            hasStarMousePosition = true;
        }
        if(env->input->pressed(Input::MOUSE_BUTTON_MIDDLE)){
            startMousePosition = mousePosition;
            hasStarMousePosition = true;
        }
        if(env->input->down(Input::MOUSE_BUTTON_RIGHT)){
            if (!hasStarMousePosition) {
                startMousePosition = mousePosition;
                hasStarMousePosition = true;
            }
            if(camera.type == Camera::PERSPECTIVE) {
                //lock around
                glm::vec2 delta = mousePosition - startMousePosition;
                transform.rotation.y -= delta.x * 0.001 / transform.scale.z;
                transform.rotation.x -= delta.y * 0.001 / transform.scale.z;
                transform.rotation.x = glm::radians( std::min(89.0f, std::max(-89.0f, glm::degrees(transform.rotation.x))));
                transform.rotation.z = 0;
                env->input->setMousePosition(startMousePosition);
            }
        }
        if(env->input->down(Input::MOUSE_BUTTON_MIDDLE)){
            if (!hasStarMousePosition) {
                startMousePosition = mousePosition;
                hasStarMousePosition = true;
            }
            //pan move
            glm::vec2 delta = mousePosition - startMousePosition;
            position += delta.y * camera.up * 0.001f * speed * transform.scale.y;
            position -= delta.x * camera.right * 0.001f * speed * transform.scale.x;
            env->input->setMousePosition(startMousePosition);
        }

        if (transform.parent == -1) {
            transform.setWorldPosition(position);
        }
        else {
            Transform *parent = env->world->getComponent<Transform>(transform.parent);
            if (parent) {
                parent->setWorldPosition(position - glm::vec3(parent->getMatrix() * glm::vec4(transform.position, 0)));
            }
            else {
                transform.setWorldPosition(position);
            }
        }


    }

}

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Transform.h"
#include "tridot/engine/Engine.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <imguizmo/ImGuizmo.h>

namespace tridot {

    glm::mat4 Transform::getMatrix() const {
        return parent.matrix * getLocalMatrix();
    }

    glm::mat4 Transform::getLocalMatrix() const {
        glm::mat4 transform(1);
        transform = glm::translate(transform, position);
        if(rotation != glm::vec3(0, 0, 0)){
            transform = glm::rotate(transform, rotation.z, {0, 0, 1});
            transform = glm::rotate(transform, rotation.y, {0, 1, 0});
            transform = glm::rotate(transform, rotation.x, {1, 0, 0});
        }
        transform = glm::scale(transform, scale);
        return transform;
    }

    Transform Transform::decompose(const glm::mat4 &matrix) {
        Transform transform;
        ImGuizmo::DecomposeMatrixToComponents((float*)&matrix, (float*)&transform.position, (float*)&transform.rotation, (float*)&transform.scale);
        transform.rotation = glm::radians(transform.rotation);
        return transform;
    }

    TRI_INIT("transform"){
        engine.onUpdate().order({"transform", "rendering"});
    }

    TRI_UPDATE("transform"){
        engine.view<Transform>().each([](Transform &t){
            if(t.parent.id != -1){
                if(engine.has<Transform>(t.parent.id)){
                    Transform &pt = engine.get<Transform>(t.parent.id);
                    t.parent.matrix = pt.getMatrix();
                }
            }else{
                t.parent.matrix = glm::mat4(1);
            }
        });
    }

}
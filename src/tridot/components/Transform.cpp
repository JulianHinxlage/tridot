//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Transform.h"
#include "tridot/core/Environment.h"
#include "tridot/engine/engine.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <imguizmo/ImGuizmo.h>

namespace tridot {

    std::map<EntityId, std::vector<EntityId>> Transform::children;

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

    void Transform::decompose(const glm::mat4 &matrix) {
        ImGuizmo::DecomposeMatrixToComponents((float*)&matrix, (float*)&position, (float*)&rotation, (float*)&scale);
        rotation = glm::radians(rotation);
    }

    TRI_INIT_CALLBACK("transform"){
        env->events->entityDestroy.addCallback("transform", [](Registry *reg, EntityId id){
            if(reg == env->scene){
                if(Transform::hasChildren(id)){
                    for(EntityId child : Transform::getChildren(id)){
                        reg->destroy(child);
                    }
                }
            }
        });
    }

    TRI_UPDATE_CALLBACK("transform"){
        TRI_PROFILE("transforms");
        Transform::children.clear();
        env->scene->view<Transform>().each([](EntityId id, Transform &t){
            if(t.parent.id != -1){
                Transform::children[t.parent.id].push_back(id);
                if(env->scene->has<Transform>(t.parent.id)){
                    Transform &pt = env->scene->get<Transform>(t.parent.id);
                    t.parent.matrix = pt.getMatrix();
                }
            }else{
                t.parent.matrix = glm::mat4(1);
            }
        });
    }

    const std::vector<EntityId> &Transform::getChildren(EntityId id) {
        return Transform::children[id];
    }

    bool Transform::hasChildren(EntityId id) {
        return Transform::children.contains(id);
    }

}
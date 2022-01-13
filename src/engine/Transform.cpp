//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//


#include "Transform.h"
#include "core/core.h"
#include "entity/Scene.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <imguizmo/ImGuizmo.h>

namespace tri {

    glm::mat4 Transform::calculateLocalMatrix() const {
        glm::mat4 transform(1);
        transform = glm::translate(transform, position);
        if (rotation.z != 0) {
            transform = glm::rotate(transform, rotation.z, { 0, 0, 1 });
        }
        if (rotation.y != 0) {
            transform = glm::rotate(transform, rotation.y, {0, 1, 0});
        }
        if (rotation.x != 0) {
            transform = glm::rotate(transform, rotation.x, {1, 0, 0});
        }
        transform = glm::scale(transform, scale);
        return transform;
    }

    const glm::mat4 &Transform::getMatrix() const {
        return matrix;
    }

    void Transform::decompose(const glm::mat4 &matrix) {
        ImGuizmo::DecomposeMatrixToComponents((float*)&matrix, (float*)&position, (float*)&rotation, (float*)&scale);
        rotation = glm::radians(rotation);
    }

    TRI_REGISTER_COMPONENT(Transform);
    TRI_REGISTER_MEMBER(Transform, position);
    TRI_REGISTER_MEMBER(Transform, scale);
    TRI_REGISTER_MEMBER(Transform, rotation);
    TRI_REGISTER_MEMBER(Transform, parent);

    TRI_REGISTER_COMPONENT(NoHierarchyUpdate)

    TRI_REGISTER_SYSTEM_INSTANCE(HierarchySystem, env->hierarchies);

    const std::vector<EntityId> &HierarchySystem::getChildren(EntityId id) {
        auto x = children.find(id);
        if(x == children.end()){
            return empty;
        }else{
            return x->second;
        }
    }

    bool HierarchySystem::isParentOf(EntityId id, EntityId parent){
        if(id == parent){
            return true;
        }
        if(env->scene->hasComponent<Transform>(id)){
            Transform &t = env->scene->getComponent<Transform>(id);
            if(t.parent == parent){
                return true;
            }
            if(t.parent != -1){
                return isParentOf(t.parent, parent);
            }
        }
        return false;
    }

    bool HierarchySystem::haveSameParent(EntityId id1, EntityId id2){
        if(id1 == id2){
            return true;
        }
        int parent1 = -1;
        int parent2 = -1;
        if(env->scene->hasComponent<Transform>(id1)){
            Transform &t = env->scene->getComponent<Transform>(id1);
            parent1 = t.parent;
        }
        if(env->scene->hasComponent<Transform>(id2)){
            Transform &t = env->scene->getComponent<Transform>(id2);
            parent2 = t.parent;
        }
        return parent1 == parent2;
    }

    void HierarchySystem::update() {
        children = childrenBase;
        auto *pool = env->scene->getComponentPool<Transform>();
        env->scene->view<Transform>().except<NoHierarchyUpdate>().each([&](EntityId id, Transform &t){
            if(t.parent != -1){
                children[t.parent].push_back(id);
                if(env->scene->hasComponent<Transform>(t.parent)){
                    Transform &parent = env->scene->getComponent<Transform>(t.parent);
                    t.matrix = parent.getMatrix() * t.calculateLocalMatrix();


                    //update order control
                    EntityId parentId = t.parent;
                    int index1 = pool->getIndexById(id);
                    int index2 = pool->getIndexById(parentId);
                    while(index2 > index1){
                        pool->swap(index2, index2 - 1);
                        index2 = pool->getIndexById(parentId);
                    }

                }else{
                    t.matrix = t.calculateLocalMatrix();
                }
            }else{
                t.matrix = t.calculateLocalMatrix();
            }
        });
    }

    void HierarchySystem::startup() {
        env->signals->entityRemove.addCallback([&](EntityId id, Scene *scene){
            if(scene == env->scene){
                for(auto &child : getChildren(id)){
                    scene->removeEntity(child);
                }
            }
        });
        env->signals->getComponentInit<NoHierarchyUpdate>().addCallback([&](EntityId id, Scene *scene) {
            if (scene->hasComponent<Transform>(id)) {
                Transform &t = scene->getComponent<Transform>(id);
                if (t.parent != -1) {
                    childrenBase[t.parent].push_back(id);
                }
                t.matrix = t.calculateLocalMatrix();
            }
        });
        env->signals->getComponentShutdown<NoHierarchyUpdate>().addCallback([&](EntityId id, Scene* scene) {
            if (scene->hasComponent<Transform>(id)) {
                Transform& t = scene->getComponent<Transform>(id);
                if (t.parent != -1) {
                    auto& list = childrenBase[t.parent];
                    for (int i = 0; i < list.size(); i++) {
                        if (list[i] == t.parent) {
                            list.erase(list.begin() + i);
                            break;
                        }
                    }
                }
            }
        });
    }

}

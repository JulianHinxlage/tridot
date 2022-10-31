//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//


#include "Transform.h"
#include "core/core.h"
#include "entity/World.h"
#include "engine/Time.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtx/matrix_decompose.hpp>

namespace tri {

    Transform::Transform(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& rotation)
        : position(position), scale(scale), rotation(rotation) {
        parent = -1;
        matrix = glm::mat4(0);
    }

    glm::vec3 Transform::getWorldPosition() const {
        Transform t;
        t.decompose(getMatrix());
        return t.position;
    }

    glm::vec3 Transform::getWorldScale() const {
        Transform t;
        t.decompose(getMatrix());
        return t.scale;
    }

    glm::vec3 Transform::getWorldRotation() const {
        Transform t;
        t.decompose(getMatrix());
        return t.rotation;
    }

    void Transform::setWorldPosition(const glm::vec3& position) {
        if (parent == -1) {
            this->position = position;
        }
        else {
            glm::mat4 parentMatrix = getMatrix() * glm::inverse(calculateLocalMatrix());
            this->position = glm::inverse(parentMatrix) * glm::vec4(position, 1);
        }
    }

    void Transform::setWorldScale(const glm::vec3& scale) {
        glm::mat4 parentMatrix = getMatrix() * glm::inverse(calculateLocalMatrix());
        Transform t;
        t.decompose(glm::inverse(parentMatrix) * glm::scale(glm::mat4(1), scale));
        this->scale = t.scale;
    }

    void Transform::setWorldRotation(const glm::vec3& rotation) {
        glm::mat4 parentMatrix = getMatrix() * glm::inverse(calculateLocalMatrix());
        Transform t;

        glm::mat4 rot(1);
        if (rotation.z != 0) {
            rot = glm::rotate(rot, rotation.z, { 0, 0, 1 });
        }
        if (rotation.y != 0) {
            rot = glm::rotate(rot, rotation.y, { 0, 1, 0 });
        }
        if (rotation.x != 0) {
            rot = glm::rotate(rot, rotation.x, { 1, 0, 0 });
        }

        t.decompose(glm::inverse(parentMatrix) * rot);
        this->rotation = t.rotation;
    }

    glm::mat4 Transform::calculateLocalMatrix() const {
        glm::mat4 transform(1);
        transform = glm::translate(transform, position);
        if (rotation.z != 0) {
            transform = glm::rotate(transform, rotation.z, {0, 0, 1});
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

    void Transform::setMatrix(const glm::mat4& matrix) {
        this->matrix = matrix;
    }

    void Transform::decompose(const glm::mat4 &matrix) {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::quat orentiation;
        glm::decompose(matrix, scale, orentiation, position, skew, perspective);
        rotation = glm::eulerAngles(orentiation);
    }

    TRI_COMPONENT(Transform);
    TRI_PROPERTIES3(Transform, position, scale, rotation);
    TRI_PROPERTY_FLAGS(Transform, parent, PropertyDescriptor::HIDDEN);

    class STransform : public System {
    public:
        std::unordered_map<EntityId, std::vector<EntityId>> childs;
        std::unordered_map<EntityId, std::vector<EntityId>> newChilds;
        std::vector<EntityId> empty;
        int listener = -1;
        int listener2 = -1;

        void init() override {
            auto* job = env->jobManager->addJob("Transform");
            job->addSystem<STransform>();
            job->enableMultithreading = true;

            listener = env->eventManager->postTick.addListener([&]() {
                childs.swap(newChilds);
            });
            listener2 = env->eventManager->onEntityRemove.addListener([&](World *world, EntityId id) {
                if (world == env->world) {
                    for (auto child : getChilds(id)) {
                        Transform* t = env->world->getComponent<Transform>(child);
                        if (t) {
                            t->parent = -1;
                        }
                    }
                    childs.erase(id);
                    Transform* t = env->world->getComponent<Transform>(id);
                    if (t && t->parent != -1) {
                        childs[t->parent].clear();
                    }
                }
            });
            env->eventManager->onEntityAdd.addListener([](World* world, EntityId id) {
                Transform* t = env->world->getComponent<Transform>(id);
                if (t) {
                    t->setMatrix(t->calculateLocalMatrix());
                }
            });
        }

        void shutdown() override {
            env->eventManager->postTick.removeListener(listener);
            env->eventManager->onEntityRemove.removeListener(listener2);
        }

        void tick() override {
            TRI_PROFILE_FUNC();
            newChilds.clear();
            std::vector<EntityId> stack;
            std::vector<glm::mat4> matStack;
            glm::mat4 parentMat(1);
            env->world->each<const Transform>([&](EntityId tid, Transform& t) {
                if (t.parent == -1) {

                    t.matrix = t.calculateLocalMatrix();
                    parentMat = t.matrix;
                    for (auto child : getChilds(tid)) {
                        stack.push_back(child);
                    }

                    while (!stack.empty()) {
                        EntityId id = stack.back();
                        stack.pop_back();

                        if (id == -1) {
                            parentMat = matStack.back();
                            matStack.pop_back();
                            continue;
                        }

                        Transform* t = env->world->getComponent<Transform>(id);
                        auto &childs = getChilds(id);
                        if (t) {
                            t->matrix = parentMat * t->calculateLocalMatrix();
                            if (childs.size() > 0) {
                                matStack.push_back(parentMat);
                                parentMat = t->matrix;
                                stack.push_back(-1);
                            }
                            for (auto child : childs) {
                                stack.push_back(child);
                            }
                        }
                    }

                }
                else {
                    newChilds[t.parent].push_back(tid);
                }
            });
        }

        const std::vector<EntityId>& getChilds(EntityId id) {
            auto entry = childs.find(id);
            if (entry == childs.end()) {
                return empty;
            }
            else {
                return entry->second;
            }
        }
    };
    TRI_SYSTEM(STransform);

    const std::vector<EntityId>& Transform::getChilds(EntityId id) {
        return env->systemManager->getSystem<STransform>()->getChilds(id);
    }

}

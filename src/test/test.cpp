//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "core/core.h"
#include "engine/Transform.h"
#include "entity/World.h"
#include "entity/Prefab.h"
#include "engine/Time.h"
#include "core/ThreadManager.h"
#include "engine/Random.h"
#include "engine/MeshComponent.h"
#include "engine/EntityEvent.h"
#include "engine/RuntimeMode.h"

namespace tri {

    //Test
    class Movement {
    public:
        glm::vec3 speed;
        glm::vec3 initialPos;
    };
    TRI_COMPONENT_CATEGORY(Movement, "Test");
    TRI_PROPERTIES2(Movement, speed, initialPos);

    class TestComp {
    public:
        float value = 0;
        std::vector<int> ints;
        EntityEvent event;

        void random() {
            value = env->random->getFloat();
        }
    };
    TRI_COMPONENT_CATEGORY(TestComp, "Test");
    TRI_PROPERTIES3(TestComp, value, ints, event);
    TRI_PROPERTY_RANGE(TestComp, value, 0, 1);
    TRI_FUNCTION(TestComp, random);

    class Acceleration {
    public:
        glm::vec3 acceleration;
    };
    TRI_COMPONENT_CATEGORY(Acceleration, "Test");
    TRI_PROPERTIES1(Acceleration, acceleration);


    class TestManager {
    public:
        void spawn() {
            for (int i = 0; i < 10000; i++) {
                float x = i % 10;
                float y = (i / 10) % 10;
                float z = (i / 100) % 10;

                auto id = env->world->addEntity(
                    Transform(glm::vec3(x, y, z) * 0.5f)
                );
            }
        }
        void spawnHierarchy() {
            std::vector<EntityId> ids;

            for (int i = 0; i < 10000; i++) {
                float x = i % 10;
                float y = (i / 10) % 10;
                float z = (i / 100) % 10;

                Transform t(glm::vec3(x, y, z) * 0.5f);
                if (ids.size() > 0) {
                    t.parent = ids[i / 10];
                }
                auto id = env->world->addEntity(t);

                if (env->random->getFloat() < 0.002) {
                    env->world->addComponent<MeshComponent>(id);
                }

                ids.push_back(id);
            }
        }
        void spawnMove() {
            std::vector<EntityId> ids;

            for (int i = 0; i < 10000; i++) {
                float x = i % 10;
                float y = (i / 10) % 10;
                float z = (i / 100) % 10;

                Transform t(glm::vec3(x, y, z) * 0.5f);
                if (ids.size() > 0) {
                    t.parent = ids[i / 10];
                }
                Movement m;
                m.speed = env->random->getVec3() - 0.5f * 0.2f;
                auto id = env->world->addEntity(t, m);

                if (env->random->getFloat() < 0.002) {
                    env->world->addComponent<MeshComponent>(id);
                }

                ids.push_back(id);
            }
        }
    };
    TRI_COMPONENT_CATEGORY(TestManager, "Test");
    TRI_FUNCTION(TestManager, spawn);
    TRI_FUNCTION(TestManager, spawnHierarchy);
    TRI_FUNCTION(TestManager, spawnMove);

	class Test : public System {
    public:
        int onComponnetAddListener = 0;

        void init() override {
            onComponnetAddListener = env->eventManager->onComponentAdd<Movement>().addListener([](World *world, EntityId id) {
                auto* m = world->getComponent<Movement>(id);
                if (auto* t = world->getComponent<Transform>(id)) {
                    m->initialPos = t->position;
                }
            });
            env->jobManager->addJob("Test", { "Test" });
        }

        void shutdown() override {
            env->eventManager->onComponentAdd<Movement>().removeListener(onComponnetAddListener);
        }

        void startup() override {
            TRI_PROFILE_FUNC();

            Clock clock;
            env->world->enablePendingOperations = false;

            int count = 0;
            int test = 0;
            if (test == 0) {
                env->world->getEntityStorage()->reserve(count);
                env->world->getComponentStorage<Transform>()->reserve(count / 2 + 1);
                env->world->getComponentStorage<Movement>()->reserve(count / 3 + 1);
                env->world->getComponentStorage<Acceleration>()->reserve(count / 4 + 1);

                for (int i = 0; i < count; i++) {
                    auto id = env->world->addEntity();
                    if (i % 3 == 0) {
                        env->world->addComponent(id, Movement({i, i, i}));
                    }
                    if (i % 2 == 0) {
                        env->world->addComponent<Transform>(id).position = glm::vec3(i, i, i) * 0.5f;
                    }
                    if (i % 4 == 0) {
                        env->world->addComponent<Acceleration>(id).acceleration = { i, i, i };
                    }
                    
                }
            }
            else if (test == 1) {
                env->world->getEntityStorage()->reserve(166667);
                env->world->getComponentStorage<Transform>()->reserve(500000);
                env->world->getComponentStorage<Movement>()->reserve(333334);

                for (int i = 0; i < 166667; i++) {
                    auto id = env->world->addEntity();
                    env->world->addComponent<Transform>(id).position = glm::vec3(i, i, i) * 0.5f;
                    env->world->addComponent<Movement>(id).speed = { i, i, i };
                }

                for (int i = 166667; i < 333334; i++) {
                    auto id = env->world->addEntity();
                    env->world->addComponent<Movement>(id).speed = { i, i, i };
                }

                for (int i = 333334; i < 166667 + 500000; i++) {
                    auto id = env->world->addEntity();
                    env->world->addComponent<Transform>(id).position = glm::vec3(i, i, i) * 0.5f;
                }

                for (int i = 166667 + 500000; i < 1000000; i++) {
                    auto id = env->world->addEntity();
                }
            }
            else if (test == 2) {
                env->world->getEntityStorage()->reserve(count);
                env->world->getComponentStorage<Transform>()->reserve(count / 6);
                env->world->getComponentStorage<Movement>()->reserve(count / 6);
                env->world->getComponentStorage<Acceleration>()->reserve(count / 6);

                for (int i = 0; i < count / 6; i++) {
                    auto id = env->world->addEntity(Transform(glm::vec3(i, i, i) * 0.5f), Movement({ i, i, i }));
                    if (i < count / 12) {
                        env->world->addComponent<Acceleration>(id).acceleration = { i, i, i };
                    }
                }
                for (int i = count / 6; i < count; i++) {
                    auto id = env->world->addEntity();
                }
            }
            else if (test == 3) {
                for (int i = 0; i < count / 6; i++) {
                    auto id = env->world->addEntity(Transform(glm::vec3(i, i, i) * 0.5f), Movement({ i, i, i }));
                    if (i < count / 12) {
                        env->world->addComponent<Acceleration>(id).acceleration = { i, i, i };
                    }
                }
            }
            env->world->enablePendingOperations = true;


            /*
            float createTime = clock.elapsed();
            clock.reset();
            env->console->info("create:  %f ms", createTime * 1000.0f);
            

            env->world->setComponentGroup<Transform, Movement>();
            env->world->setComponentGroup<Transform, Movement, Acceleration>();
            float groupTime = clock.elapsed();
            clock.reset();
            env->console->info("group:   %f ms", groupTime * 1000.0f);


            env->world->enablePendingOperations = true;
            env->world->performePending();
            float pendingTime = clock.elapsed();


            env->console->info("pending: %f ms", pendingTime * 1000.0f);
            env->console->info("total:   %f ms", (createTime + pendingTime + groupTime) * 1000.0f);
            */
        }

        void tick() override {
            int count1 = 0;
            int count2 = 0;

            bool valid1 = true;
            bool valid2 = true;

            env->world->view<Transform, const Movement>().each([&](Transform& t, Movement& m) {

                if (env->world->getIdByComponent(&t) != env->world->getIdByComponent(&m)) {
                    valid1 = false;
                }

                t.position += m.speed * env->time->deltaTime;
                count1++;
            });

            env->world->view<const Transform, Movement, const Acceleration>().each([&](Transform& t, Movement& m, Acceleration &a) {

                if (env->world->getIdByComponent(&t) != env->world->getIdByComponent(&m)) {
                    valid2 = false;
                }
                if (env->world->getIdByComponent(&t) != env->world->getIdByComponent(&a)) {
                    valid2 = false;
                }

                m.speed += a.acceleration * env->time->deltaTime;
                count2++;
            });

            //if (env->time->frameTicks(1.0)) {
            //    env->console->info("FPS: %.2f (%.2f ms)", env->time->framesPerSecond, env->time->avgFrameTime * 1000);
            //    env->console->info("count1: %i", (int)count1);
            //    env->console->info("count2: %i", (int)count2);
            //    env->console->info("valid1: %i", (int)valid1);
            //    env->console->info("valid2: %i", (int)valid2);
            //}


            //add entities
            //for (int i = 0; i < 1000; i++) {
            //    auto id = env->world->addEntity();
            //    if (i % 3 == 0) {
            //        env->world->addComponent(id, Movement({ i, i, i }));
            //    }
            //    if (i % 2 == 0) {
            //        env->world->addComponent<Transform>(id).position = glm::vec3(i, i, i) * 0.5f;
            //    }
            //    if (i % 4 == 0) {
            //        env->world->addComponent<Acceleration>(id).acceleration = { i, i, i };
            //    }
            //
            //}

        }
	};
	TRI_SYSTEM(Test);


    class Test2 : public System {
    public:
        World *copy = nullptr;

        void init() override {
            env->jobManager->addJob("Test", { "Test2" });
            env->runtimeMode->setActiveSystem(RuntimeMode::EDIT, "Test2", true);

            //env->eventManager->postTick.addListener([&]() {
            //    if (env->time->frameTicks(2.0)) {
            //        TRI_PROFILE("copy world");
            //        if (!copy) {
            //            copy = new World;
            //            copy->copy(*env->world);
            //        }
            //        else {
            //            env->world->copy(*copy);
            //        }
            //    }
            //});

        }
        void tick() override {
            //env->world->each<Transform>([&](Transform& t) {
            //    //t.position -= m.speed * env->time->deltaTime;
            //});

            //if (env->time->time > 5.0f) {
            //    int* ptr = nullptr;
            //    *ptr = 0;
            //}
        }
    };
    TRI_SYSTEM(Test2);

}
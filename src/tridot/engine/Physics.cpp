//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Physics.h"
#include <vector>
#include <glm/detail/type_quat.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <BulletDynamics/Dynamics/btSimpleDynamicsWorld.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <LinearMath/btDefaultMotionState.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>

namespace tridot {

    btVector3 conv(const glm::vec3 &vec){
        return btVector3(vec.x, vec.y, vec.z);
    }

    glm::vec3 conv(const btVector3 &vec){
        return glm::vec3(vec.getX(), vec.getY(), vec.getZ());
    }

    btQuaternion convQuaternion(const glm::vec3 &euler){
        glm::quat quaternion(-euler);
        return btQuaternion(-quaternion.x, -quaternion.y, -quaternion.z, quaternion.w);
    }

    glm::vec3 convQuaternion(const btQuaternion &quat){
        glm::quat quaternion(quat.getW(), -quat.getX(), -quat.getY(), -quat.getZ());
        return -glm::vec3(glm::eulerAngles(quaternion));
    }

    class Physics::Impl{
    public:
        btDynamicsWorld *world;
        btBroadphaseInterface* broadphase;
        btCollisionDispatcher* dispatcher;
        btSequentialImpulseConstraintSolver* solver;
        btDefaultCollisionConfiguration* collisionConfiguration;

        std::vector<btCollisionShape*> shapes;
        std::vector<btRigidBody*> bodies;

        ~Impl(){
            clear();
        }

        void init(){
            collisionConfiguration = new btDefaultCollisionConfiguration();
            dispatcher = new btCollisionDispatcher(collisionConfiguration);
            broadphase = new btDbvtBroadphase();
            solver = new btSequentialImpulseConstraintSolver();
            world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
        }

        void clear(){
            if (world != nullptr){
                for(int i = world->getNumConstraints() - 1; i >= 0; i--){
                    world->removeConstraint(world->getConstraint(i));
                }
                for(int i = world->getNumCollisionObjects() - 1; i >= 0; i--){
                    btCollisionObject *obj = world->getCollisionObjectArray()[i];
                    btRigidBody* body = btRigidBody::upcast(obj);
                    if (body && body->getMotionState()){
                        delete body->getMotionState();
                    }
                    world->removeCollisionObject(obj);
                }
            }

            for(int i = 0; i < bodies.size(); i++){
                delete bodies[i];
            }
            bodies.clear();
            for(int i = 0; i < shapes.size(); i++){
                delete shapes[i];
            }
            shapes.clear();

            delete world;
            delete solver;
            delete broadphase;
            delete dispatcher;
            delete collisionConfiguration;
        }
    };

    Physics::Physics() {
        impl = nullptr;
    }

    Physics::~Physics() {}

    void Physics::init(glm::vec3 gravity) {
        impl = Ref<Impl>::make();
        impl->init();
        impl->world->setGravity(conv(gravity));
    }

    void Physics::step(float deltaTime) {
        if(impl && impl->world){
            impl->world->stepSimulation(deltaTime);
        }
    }

    void Physics::update(RigidBody &rb, Transform &t, Collider &collider, int index) {
        if(impl && impl->world) {
            if(rb.ref == nullptr){
                add(rb, t, collider, index);
            }else{
                btRigidBody* body = (btRigidBody*)rb.ref;
                btTransform &transform = body->getWorldTransform();
                glm::vec3 position = conv(transform.getOrigin());
                glm::vec3 rotation = convQuaternion(transform.getRotation());
                glm::vec3 velocity = conv(body->getLinearVelocity());
                glm::vec3 angular = conv(body->getAngularVelocity());

                if(rb.lastVelocity != rb.velocity){
                    body->setActivationState(ACTIVE_TAG);
                }

                t.position = position + (t.position - rb.lastPosition);
                t.rotation = rotation + (t.rotation - rb.lastRotation);
                rb.velocity = velocity + (rb.velocity - rb.lastVelocity);
                rb.angular = angular + (rb.angular - rb.lastAngular);

                transform.setOrigin(conv(t.position));
                transform.setRotation(convQuaternion(t.rotation));
                body->setLinearVelocity(conv(rb.velocity));
                body->setAngularVelocity(conv(rb.angular));

                rb.lastPosition = t.position;
                rb.lastRotation = t.rotation;
                rb.lastVelocity = rb.velocity;
                rb.lastAngular = rb.angular;
            }
        }
    }

    void Physics::add(RigidBody &rb, Transform &t, Collider &collider, int index) {
        if(impl && impl->world) {
            if(rb.ref == nullptr){

                btVector3 localInertia(0, 0, 0);
                btCollisionShape *shape;

                if(collider.type == Collider::SPHERE){
                    btSphereShape* s = new btSphereShape(collider.scale.x * t.scale.x * 0.5f);
                    if (rb.mass != 0){
                        s->calculateLocalInertia(rb.mass, localInertia);
                    }
                    shape = s;
                }else if(collider.type == Collider::BOX){
                    btBoxShape* s = new btBoxShape(conv(collider.scale * t.scale * 0.5f));
                    if (rb.mass != 0){
                        s->calculateLocalInertia(rb.mass, localInertia);
                    }
                    shape = s;
                }


                btTransform transform;
                transform.setIdentity();
                transform.setOrigin(conv(t.position));
                transform.setRotation(convQuaternion(t.rotation));

                btDefaultMotionState* motionState = new btDefaultMotionState(transform);
                btRigidBody::btRigidBodyConstructionInfo info(rb.mass, motionState, shape, localInertia);
                btRigidBody* body = new btRigidBody(info);
                body->setFriction(rb.friction);
                body->setRestitution(rb.restitution);
                body->setRollingFriction(rb.friction / 100);
                body->setSpinningFriction(rb.friction / 100);

                body->setAngularVelocity(conv(rb.angular));
                body->setLinearVelocity(conv(rb.velocity));
                body->setDamping(rb.linearDamping, rb.angularDamping);

                body->setCcdMotionThreshold(1e-7);
                body->setCcdSweptSphereRadius(0.2);

                body->setUserIndex(index);
                impl->world->addRigidBody(body);
                rb.ref = (void*)body;
                impl->shapes.push_back(shape);
                impl->bodies.push_back(body);

                rb.lastPosition = t.position;
                rb.lastRotation = t.rotation;
                rb.lastVelocity = rb.velocity;
                rb.lastAngular = rb.angular;
            }
        }
    }

    void Physics::remove(RigidBody &rb) {
        if(impl && impl->world) {
            if(rb.ref != nullptr) {
                btRigidBody* body = (btRigidBody*)rb.ref;
                impl->world->removeRigidBody(body);
                delete body->getMotionState();
                for(int i = 0; i < impl->bodies.size(); i++){
                    if(impl->bodies[i] == body){
                        impl->bodies.erase(impl->bodies.begin() + i);
                        impl->shapes.erase(impl->shapes.begin() + i);
                        break;
                    }
                }
            }
        }
    }

    void Physics::rayCast(glm::vec3 from, glm::vec3 to, bool firstOnly, std::function<void(const glm::vec3 &pos, int index)> callback) {
        if(firstOnly){
            btCollisionWorld::ClosestRayResultCallback results(conv(from), conv(to));
            impl->world->rayTest(conv(from), conv(to), results);
            if(results.hasHit()){
                glm::vec3 pos = conv(conv(from).lerp(conv(to), results.m_closestHitFraction));
                callback(pos, results.m_collisionObject->getUserIndex());
            }
        }else {
            btCollisionWorld::AllHitsRayResultCallback results(conv(from), conv(to));
            impl->world->rayTest(conv(from), conv(to), results);
            for (int i = 0; i < results.m_hitFractions.size(); i++) {
                glm::vec3 pos = conv(conv(from).lerp(conv(to), results.m_hitFractions[i]));
                callback(pos, results.m_collisionObjects[i]->getUserIndex());
            }
        }
    }

    class ContactCallback : public btCollisionWorld::ContactResultCallback{
    public:
        btRigidBody *source;
        std::vector<std::pair<glm::vec3, btCollisionObject*>> contacts;

        virtual btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1){
            if(colObj0Wrap->m_collisionObject == source){
                contacts.push_back({(glm::vec3)conv(cp.getPositionWorldOnA()), (btCollisionObject*)colObj1Wrap->m_collisionObject});
            }else{
                contacts.push_back({(glm::vec3)conv(cp.getPositionWorldOnB()), (btCollisionObject*)colObj0Wrap->m_collisionObject});
            }
            return 0;
        }
    };

    void Physics::contacts(RigidBody &rb, std::function<void(const glm::vec3 &, int)> callback) {
        ContactCallback result;
        result.source = (btRigidBody*)rb.ref;
        impl->world->contactTest((btRigidBody*)rb.ref, result);
        for(auto &c : result.contacts){
            callback(c.first, c.second->getUserIndex());
        }
    }

}
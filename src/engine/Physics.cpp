//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Physics.h"
#include "Transform.h"
#include "core/core.h"
#include "core/ThreadPool.h"
#include "engine/Time.h"
#include "entity/Scene.h"

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

namespace tri {

	TRI_REGISTER_SYSTEM_INSTANCE(Physics, env->physics);

	btVector3 conv(const glm::vec3& vec) {
		return btVector3(vec.x, vec.y, vec.z);
	}

	glm::vec3 conv(const btVector3& vec) {
		return glm::vec3(vec.getX(), vec.getY(), vec.getZ());
	}

	btQuaternion convQuaternion(const glm::vec3& euler) {
		glm::quat quaternion(euler);
		return btQuaternion(quaternion.x, quaternion.y, quaternion.z, quaternion.w);
	}

	glm::vec3 convQuaternion(const btQuaternion& quat) {
		glm::quat quaternion(quat.getW(), quat.getX(), quat.getY(), quat.getZ());
		return glm::vec3(glm::eulerAngles(quaternion));
	}

	class Physics::Impl {
	public:
		btDynamicsWorld* world;
		btBroadphaseInterface* broadphase;
		btCollisionDispatcher* dispatcher;
		btSequentialImpulseConstraintSolver* solver;
		btDefaultCollisionConfiguration* collisionConfiguration;
		Clock clock;

		std::vector<btCollisionShape*> shapes;
		std::vector<btRigidBody*> bodies;

		void init() {
			collisionConfiguration = new btDefaultCollisionConfiguration();
			dispatcher = new btCollisionDispatcher(collisionConfiguration);
			broadphase = new btDbvtBroadphase();
			solver = new btSequentialImpulseConstraintSolver();
			world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
			world->setGravity(btVector3(0, -9.81, 0));
			clock.reset();
		}

		void tick() {
			if (world) {
				world->stepSimulation(clock.round() * env->time->deltaTimeFactor, 6, 1 / 60.0f);
			}
		}

		void clear() {
			if (world != nullptr) {
				for (int i = world->getNumConstraints() - 1; i >= 0; i--) {
					world->removeConstraint(world->getConstraint(i));
				}
				for (int i = world->getNumCollisionObjects() - 1; i >= 0; i--) {
					btCollisionObject* obj = world->getCollisionObjectArray()[i];
					btRigidBody* body = btRigidBody::upcast(obj);
					if (body && body->getMotionState()) {
						delete body->getMotionState();
					}
					world->removeCollisionObject(obj);
				}
			}

			for (int i = 0; i < bodies.size(); i++) {
				delete bodies[i];
			}
			bodies.clear();
			for (int i = 0; i < shapes.size(); i++) {
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

	void Physics::addRigidBody(EntityId &id, RigidBody& rigidBody, Collider &collider, Transform& transform) {
		btVector3 localInertia(0, 0, 0);
		btCollisionShape* shape;

		float mass = rigidBody.mass;
		if (rigidBody.type == RigidBody::STATIC) {
			mass = 0;
		}
		
		if (collider.type == Collider::SPHERE) {
			btSphereShape* s = new btSphereShape(collider.size.x * transform.scale.x * 0.5f);
			if (mass != 0) {
				s->calculateLocalInertia(mass, localInertia);
			}
			shape = s;
		}
		else if (collider.type == Collider::BOX) {
			btBoxShape* s = new btBoxShape(conv(collider.size * transform.scale * 0.5f));
			if (mass != 0) {
				s->calculateLocalInertia(mass, localInertia);
			}
			shape = s;
		}

		btTransform bodyTransform;
		bodyTransform.setIdentity();
		bodyTransform.setOrigin(conv(transform.position));
		bodyTransform.setRotation(convQuaternion(transform.rotation));

		btDefaultMotionState* motionState = new btDefaultMotionState(bodyTransform);
		btRigidBody::btRigidBodyConstructionInfo info(mass, motionState, shape, localInertia);
		btRigidBody* body = new btRigidBody(info);
		body->setFriction(rigidBody.friction);
		body->setRestitution(rigidBody.restitution);
		body->setRollingFriction(rigidBody.friction / 100);
		body->setSpinningFriction(rigidBody.friction / 100);

		body->setAngularVelocity(conv(rigidBody.angular));
		body->setLinearVelocity(conv(rigidBody.velocity));
		body->setDamping(rigidBody.linearDamping, rigidBody.angularDamping);

		body->setCcdMotionThreshold(1e-7);
		body->setCcdSweptSphereRadius(0.2);

		body->setUserIndex(id);
		impl->world->addRigidBody(body);
		rigidBody.reference = (void*)body;
		impl->shapes.push_back(shape);
		impl->bodies.push_back(body);

		rigidBody.lastPosition = transform.position;
		rigidBody.lastRotation = transform.rotation;
		rigidBody.lastVelocity = rigidBody.velocity;
		rigidBody.lastAngular = rigidBody.angular;
	}

	void Physics::removeRigidBody(EntityId& id, RigidBody& rigidBody, Collider& collider, Transform& transform) {
		if (impl && impl->world) {
			if (rigidBody.reference != nullptr) {
				btRigidBody* body = (btRigidBody*)rigidBody.reference;
				impl->world->removeRigidBody(body);
				delete body->getMotionState();
				for (int i = 0; i < impl->bodies.size(); i++) {
					if (impl->bodies[i] == body) {
						impl->bodies.erase(impl->bodies.begin() + i);
						impl->shapes.erase(impl->shapes.begin() + i);
						break;
					}
				}
				delete body;
				rigidBody.reference = nullptr;
			}
		}
	}

	void Physics::startup(){
		impl = impl.make();
		impl->init();
		threadId = env->threads->addThread([this]() {
			//impl->tick();
		});

		env->signals->sceneBegin.addCallback("Physics", [this](Scene* scene) {
			env->scene->view<RigidBody, Collider, Transform>().each([this](EntityId id, RigidBody& rb, Collider &c, Transform& t) {
				addRigidBody(id, rb, c, t);
			});
		});
		env->signals->sceneEnd.addCallback("Physics", [this](Scene* scene) {
			env->scene->view<RigidBody, Collider, Transform>().each([this](EntityId id, RigidBody& rb, Collider& c, Transform& t) {
				removeRigidBody(id, rb, c, t);
			});
		});
	}

	void Physics::update() {
		impl->tick();
		env->scene->view<RigidBody, Collider, Transform>().each([this](EntityId id, RigidBody &rigidBody, Collider &collider, Transform &transform) {
			if (!rigidBody.enablePhysics) {
				float dt = env->time->deltaTime;
				transform.position += rigidBody.velocity * dt;
				transform.rotation += rigidBody.angular * dt;
				if (rigidBody.linearDamping != 0) {
					rigidBody.velocity *= std::pow(1.0 / rigidBody.linearDamping, dt);
				}
				if (rigidBody.angularDamping != 0) {
					rigidBody.angular *= std::pow(1.0 / rigidBody.angularDamping, dt);
				}
				return;
			}
			
			if (rigidBody.reference == nullptr) {
				addRigidBody(id, rigidBody, collider, transform);
			}
			else {
				if (rigidBody.mass == 0) {
					btRigidBody* body = (btRigidBody*)rigidBody.reference;
					if (transform.position != rigidBody.lastPosition) {
						body->getWorldTransform().setOrigin(conv(transform.position));
						rigidBody.lastPosition = transform.position;
					}
					if (transform.rotation != rigidBody.lastRotation) {
						body->getWorldTransform().setRotation(convQuaternion(transform.rotation));
						rigidBody.lastRotation = transform.rotation;
					}
				}
				else {
					btRigidBody* body = (btRigidBody*)rigidBody.reference;
					btTransform bodyTransform;
					if (body->getMotionState()) {
						body->getMotionState()->getWorldTransform(bodyTransform);
					}
					else {
						bodyTransform = body->getWorldTransform();
					}
					glm::vec3 position = conv(bodyTransform.getOrigin());
					glm::vec3 rotation = convQuaternion(bodyTransform.getRotation());
					glm::vec3 velocity = conv(body->getLinearVelocity());
					glm::vec3 angular = conv(body->getAngularVelocity());

					if (rigidBody.lastVelocity != rigidBody.velocity) {
						body->setActivationState(ACTIVE_TAG);
					}

					transform.position = position + (transform.position - rigidBody.lastPosition);
					transform.rotation = rotation + (transform.rotation - rigidBody.lastRotation);
					rigidBody.velocity = velocity + (rigidBody.velocity - rigidBody.lastVelocity);
					rigidBody.angular = angular + (rigidBody.angular - rigidBody.lastAngular);

					bodyTransform.setOrigin(conv(transform.position));
					bodyTransform.setRotation(convQuaternion(transform.rotation));
					if (body->getMotionState()) {
						body->getMotionState()->setWorldTransform(bodyTransform);
					}
					else {
						body->setWorldTransform(bodyTransform);
					}
					body->setLinearVelocity(conv(rigidBody.velocity));
					body->setAngularVelocity(conv(rigidBody.angular));

					rigidBody.lastPosition = transform.position;
					rigidBody.lastRotation = transform.rotation;
					rigidBody.lastVelocity = rigidBody.velocity;
					rigidBody.lastAngular = rigidBody.angular;
				}
			}
		});
	}

	void Physics::rayCast(glm::vec3 from, glm::vec3 to, bool firstOnly, std::function<void(const glm::vec3& pos, EntityId id)> callback) {
		if (firstOnly) {
			btCollisionWorld::ClosestRayResultCallback results(conv(from), conv(to));
			impl->world->rayTest(conv(from), conv(to), results);
			if (results.hasHit()) {
				glm::vec3 pos = conv(conv(from).lerp(conv(to), results.m_closestHitFraction));
				callback(pos, results.m_collisionObject->getUserIndex());
			}
		}
		else {
			btCollisionWorld::AllHitsRayResultCallback results(conv(from), conv(to));
			impl->world->rayTest(conv(from), conv(to), results);
			for (int i = 0; i < results.m_hitFractions.size(); i++) {
				glm::vec3 pos = conv(conv(from).lerp(conv(to), results.m_hitFractions[i]));
				callback(pos, results.m_collisionObjects[i]->getUserIndex());
			}
		}
	}

	class ContactCallback : public btCollisionWorld::ContactResultCallback {
	public:
		btRigidBody* source;
		std::vector<std::pair<glm::vec3, btCollisionObject*>> contacts;

		virtual btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) {
			if (colObj0Wrap->m_collisionObject == source) {
				contacts.push_back({ (glm::vec3)conv(cp.getPositionWorldOnA()), (btCollisionObject*)colObj1Wrap->m_collisionObject });
			}
			else {
				contacts.push_back({ (glm::vec3)conv(cp.getPositionWorldOnB()), (btCollisionObject*)colObj0Wrap->m_collisionObject });
			}
			return 0;
		}
	};

	void Physics::contacts(RigidBody& rigidBody, std::function<void(const glm::vec3 &pos, EntityId id)> callback) {
		if (rigidBody.reference != nullptr) {
			ContactCallback result;
			result.source = (btRigidBody*)rigidBody.reference;
			impl->world->contactTest((btRigidBody*)rigidBody.reference, result);
			for (auto& c : result.contacts) {
				callback(c.first, c.second->getUserIndex());
			}
		}
	}

}
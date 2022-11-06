//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Physics.h"
#include "engine/RuntimeMode.h"
#include "entity/World.h"
#include "engine/Time.h"

#include <glm/detail/type_quat.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <BulletDynamics/Dynamics/btSimpleDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <LinearMath/btDefaultMotionState.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btConvexHullShape.h>
#include <BulletCollision/CollisionShapes/btConvexTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionShapes/btConcaveShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>


namespace tri {

	TRI_SYSTEM_INSTANCE(Physics, env->physics);

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
		std::vector<btCollisionShape*> shapes;
		std::vector<btRigidBody*> bodies;
		Clock clock;

		void init(glm::vec3 gravity = { 0, -9.81, 0 }) {
			collisionConfiguration = new btDefaultCollisionConfiguration();
			dispatcher = new btCollisionDispatcher(collisionConfiguration);
			broadphase = new btDbvtBroadphase();
			solver = new btSequentialImpulseConstraintSolver();
			world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
			world->setGravity(conv(gravity));
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

		static btTransform RigidBody_TransformCompoundRecursive(btCompoundShape* compound, btScalar mass)
		{
			std::vector<btScalar> massVector;
			for (int i = 0; i < compound->getNumChildShapes(); ++i) {
				massVector.push_back(mass);
			}
			btScalar* masses = massVector.data();

			// Recurse down the compound tree, transforming each compound and their children so that the
			// compound is positioned at its centre of mass and with its axes aligned with those of the
			// principal inertia tensor.
			for (int i = 0; i < compound->getNumChildShapes(); ++i)
			{
				btCollisionShape* childShape = compound->getChildShape(i);
				if (childShape->isCompound())
				{
					btCompoundShape* childCompound = static_cast<btCompoundShape*>(childShape);
					btTransform childPrincipalTransform = RigidBody_TransformCompoundRecursive(childCompound, masses[i]);
					compound->updateChildTransform(i, childPrincipalTransform * compound->getChildTransform(i)); // Transform the compound so that it is positioned at its centre of mass.
				}
			}

			// Calculate the principal transform for the compound. This has its origin at the compound's
			// centre of mass and its axes aligned with those of the inertia tensor.
			btTransform principalTransform;
			btVector3 principalInertia;
			compound->calculatePrincipalAxisTransform(masses, principalTransform, principalInertia);

			// Transform all the child shapes by the inverse of the compound's principal transform, so
			// as to restore their world positions.
			for (int i = 0; i < compound->getNumChildShapes(); ++i)
			{
				btCollisionShape* childShape = compound->getChildShape(i);
				compound->updateChildTransform(i, principalTransform.inverse() * compound->getChildTransform(i));
			}

			return principalTransform;
		}

		glm::vec3 createShape(EntityId id, float mass, btVector3 *localInertia, btCollisionShape** shape) {

			if (env->world->hasComponents<Collider, Transform>(id)) {

				Collider& collider = *env->world->getComponent<Collider>(id);
				Transform& transform = *env->world->getComponent<Transform>(id);
				Transform worldTransform;
				worldTransform.decompose(transform.getMatrix());
				glm::vec3 scale = worldTransform.scale;

				if (collider.type == Collider::SPHERE) {
					btSphereShape* s = new btSphereShape(collider.scale.x * scale.x * 0.5f);
					if (mass != 0) {
						s->calculateLocalInertia(mass, *localInertia);
					}
					*shape = s;
				}
				else if (collider.type == Collider::BOX) {
					btBoxShape* s = new btBoxShape(conv(collider.scale * scale * 0.5f));
					if (mass != 0) {
						s->calculateLocalInertia(mass, *localInertia);
					}
					*shape = s;
				}
				else if (collider.type == Collider::MESH) {
					btConvexTriangleMeshShape* s = nullptr;
					/*
					if (auto *mesh = env->world->getComponent<MeshComponent>(id)) {
						if (mesh.mesh) {
							auto& vertices = mesh.mesh->getVertexData();
							auto& indices = mesh.mesh->getIndexData();

							btTriangleMesh* m = new btTriangleMesh();

							int stride = 8;
							for (int i = 0; i < indices.size(); i += 3) {
								glm::vec3 v1(
									(float)vertices[i * stride + 0],
									(float)vertices[i * stride + 1],
									(float)vertices[i * stride + 2]
								);
								glm::vec3 v2(
									(float)vertices[(i + 1) * stride + 0],
									(float)vertices[(i + 1) * stride + 1],
									(float)vertices[(i + 1) * stride + 2]
								);
								glm::vec3 v3(
									(float)vertices[(i + 2) * stride + 0],
									(float)vertices[(i + 2) * stride + 1],
									(float)vertices[(i + 2) * stride + 2]
								);

								m->addTriangle(conv(v1 * collider.scale), conv(v2 * collider.scale), conv(v3 * collider.scale));
							}
							env->console->info("tris: ", m->getNumTriangles());
							s = new btConvexTriangleMeshShape(m);
						}
					}


					if (s != nullptr) {
						if (mass != 0) {
							s->calculateLocalInertia(mass, localInertia);
						}
						shape = s;
					}
					*/
				}
				else if (collider.type == Collider::CONCAVE_MESH) {
				}			
				
				return {0, 0, 0};
			}

			btCompoundShape* compount = nullptr;
			for (auto& child : Transform::getChilds(id)) {
				if (env->world->hasComponents<Collider, Transform>(child)) {
					if (!env->world->hasComponents<RigidBody>(child)) {
						btCollisionShape* newShape;
						btVector3 newLocalInertia;
						createShape(child, mass, &newLocalInertia, &newShape);

						if (!compount) {
							compount = new btCompoundShape();
							btTransform t;
							t.setIdentity();
							compount->addChildShape(t, *shape);
						}
						Transform& transform = *env->world->getComponent<Transform>(child);
						btTransform t;
						t.setOrigin(conv(transform.position));
						t.setRotation(convQuaternion(transform.rotation));
						compount->addChildShape(t, newShape);

					}
				}
			}
			if (compount) {
				*shape = compount;
				compount->calculateLocalInertia(mass, *localInertia);
				return { 0, 0, 0 };
			}
			else {
				return { 0, 0, 0 };
			}
		}
	};

	void Physics::init() {
		auto* job = env->jobManager->addJob("Transform");
		job->addSystem<Physics>();
		job->orderSystems({ "STransform", "Physics" });
	}

	void Physics::startup() {
		impl = std::make_shared<Impl>();
		impl->init();

		entityRemoveListener = env->eventManager->onEntityRemove.addListener([&](World* world, EntityId id) {
			if (world->hasComponents<RigidBody, Collider, Transform>(id)) {
				RigidBody* rb = world->getComponent<RigidBody>(id);
				Transform* t = world->getComponent<Transform>(id);
				removeRigidBody(id, *rb, *t);
			}
		});

		modeChangeListener = env->eventManager->onRuntimeModeChange.addListener([&](int previous, int mode) {
			if (previous == RuntimeMode::EDIT) {
				if (mode == RuntimeMode::PLAY || mode == RuntimeMode::PAUSED) {
					impl->clock.reset();
				}
			}
			if (previous == RuntimeMode::PLAY || previous == RuntimeMode::PAUSED) {
				if (mode == RuntimeMode::EDIT) {
					impl->clock.reset();

					TRI_PROFILE("removeRigidBodies");
					env->world->view<RigidBody, Transform>().each([this](EntityId id, RigidBody& rb, Transform& t) {
						removeRigidBody(id, rb, t);
					});

				}
			}
			if (mode == RuntimeMode::PLAY) {
				impl->clock.reset();
			}
			lastFrameRuntimeMode = (RuntimeMode::Mode)previous;
		});

		endMapListener = env->eventManager->onMapEnd.addListener([&](World* world, std::string mapName) {
			TRI_PROFILE("removeRigidBodies");
			env->world->view<RigidBody, Transform>().each([this](EntityId id, RigidBody& rb, Transform& t) {
				removeRigidBody(id, rb, t);
			});
		});
	}

	void Physics::shutdown() {
		env->eventManager->onEntityRemove.removeListener(entityRemoveListener);
		env->eventManager->onRuntimeModeChange.removeListener(modeChangeListener);
		env->eventManager->onMapEnd.removeListener(endMapListener);
	}

	void Physics::addRigidBody(EntityId& id, RigidBody& rigidBody, Transform& transform) {
		btVector3 localInertia(0, 0, 0);
		btCollisionShape* shape;

		float mass = rigidBody.mass;
		if (rigidBody.type == RigidBody::STATIC) {
			mass = 0;
		}

		glm::vec3 offset = impl->createShape(id, mass, &localInertia, &shape);

		btTransform bodyTransform;
		bodyTransform.setIdentity();

		Transform worldTransform;
		worldTransform.decompose(transform.getMatrix());
		bodyTransform.setOrigin(conv(worldTransform.position));
		bodyTransform.setRotation(convQuaternion(worldTransform.rotation));

		btDefaultMotionState* motionState = new btDefaultMotionState(bodyTransform);
		btRigidBody::btRigidBodyConstructionInfo info(mass, motionState, shape, localInertia);
		btRigidBody* body = new btRigidBody(info);
		if (rigidBody.type == RigidBody::KINEMATIC) {
			body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		}

		if (rigidBody.isTrigger) {
			body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
		}

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

	void Physics::removeRigidBody(EntityId& id, RigidBody& rigidBody, Transform& transform) {
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

	void Physics::tick() {
		if (auto* store = env->world->getComponentStorage<Transform>()) {
			store->lock();
		}
		if (auto* store = env->world->getComponentStorage<RigidBody>()) {
			store->lock();
		}
		if (auto* store = env->world->getComponentStorage<Collider>()) {
			store->lock();
		}

		env->world->view<RigidBody, Collider, Transform>().each([this](EntityId id, RigidBody& rigidBody, Collider& collider, Transform& transform) {
			if (rigidBody.reference) {
				btRigidBody* body = (btRigidBody*)rigidBody.reference;

				if (!rigidBody.enableGravity) {
					body->clearGravity();
				}

				Transform worldTransform;
				worldTransform.decompose(transform.getMatrix());

				if (transform.position != rigidBody.lastPosition) {
					if (rigidBody.type == RigidBody::STATIC) {
						impl->world->removeRigidBody(body);

						btTransform bodyTransform;
						bodyTransform.setIdentity();

						Transform worldTransform;
						worldTransform.decompose(transform.getMatrix());
						bodyTransform.setOrigin(conv(worldTransform.position));
						bodyTransform.setRotation(convQuaternion(worldTransform.rotation));

						body->setWorldTransform(bodyTransform);

						impl->world->addRigidBody(body);
					}
					else {
						body->getWorldTransform().setOrigin(conv(worldTransform.position));
						body->setActivationState(ACTIVE_TAG);
					}
				}
				if (transform.rotation != rigidBody.lastRotation) {
					body->getWorldTransform().setRotation(convQuaternion(worldTransform.rotation));
					body->setActivationState(ACTIVE_TAG);
				}
				if (rigidBody.velocity != rigidBody.lastVelocity) {
					body->setLinearVelocity(conv(rigidBody.velocity));
					body->setActivationState(ACTIVE_TAG);
				}
				if (rigidBody.angular != rigidBody.lastAngular) {
					body->setAngularVelocity(conv(rigidBody.angular));
					body->setActivationState(ACTIVE_TAG);
				}

				rigidBody.lastPosition = transform.position;
				rigidBody.lastRotation = transform.rotation;
				rigidBody.lastVelocity = rigidBody.velocity;
				rigidBody.lastAngular = rigidBody.angular;
			}
		});

		impl->tick();

		env->world->view<RigidBody, Transform>().each([this](EntityId id, RigidBody& rigidBody, Transform& transform) {
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
				if (lastFrameRuntimeMode != RuntimeMode::LOADING && lastFrameRuntimeMode != RuntimeMode::EDIT) {
					addRigidBody(id, rigidBody, transform);
				}
			}
			else {

				if (rigidBody.type == RigidBody::STATIC) {
					return;
				}
				if (rigidBody.type == RigidBody::KINEMATIC) {
					return;
				}

				btRigidBody* body = (btRigidBody*)rigidBody.reference;
				btTransform bodyTransform;
				if (body->getMotionState()) {
					body->getMotionState()->getWorldTransform(bodyTransform);
				}
				else {
					bodyTransform = body->getWorldTransform();
				}

				if (transform.parent != -1) {
					Transform bTransform;
					bTransform.decompose(transform.getMatrix());
					bTransform.position = conv(bodyTransform.getOrigin());
					bTransform.rotation = convQuaternion(bodyTransform.getRotation());
					glm::mat4 matrix = bTransform.calculateLocalMatrix();
					glm::mat4 parentMatrix = transform.getMatrix() * glm::inverse(transform.calculateLocalMatrix());
					transform.decompose(glm::inverse(parentMatrix) * matrix);
					transform.setMatrix(matrix);
				}
				else {
					transform.position = conv(bodyTransform.getOrigin());
					transform.rotation = convQuaternion(bodyTransform.getRotation());
				}

				rigidBody.velocity = conv(body->getLinearVelocity());
				rigidBody.angular = conv(body->getAngularVelocity());

				rigidBody.lastPosition = transform.position;
				rigidBody.lastRotation = transform.rotation;
				rigidBody.lastVelocity = rigidBody.velocity;
				rigidBody.lastAngular = rigidBody.angular;
			}
		});

		if (auto* store = env->world->getComponentStorage<Transform>()) {
			store->unlock();
		}
		if (auto* store = env->world->getComponentStorage<RigidBody>()) {
			store->unlock();
		}
		if (auto* store = env->world->getComponentStorage<Collider>()) {
			store->unlock();
		}

		lastFrameRuntimeMode = env->runtimeMode->getMode();
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

	void Physics::contacts(RigidBody& rigidBody, std::function<void(const glm::vec3& pos, EntityId id)> callback) {
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
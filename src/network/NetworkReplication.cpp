//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "NetworkReplication.h"
#include "NetworkSystem.h"
#include "core/core.h"
#include "engine/RuntimeMode.h"
#include "engine/Map.h"
#include "engine/EntityUtil.h"
#include "Packet.h"
#include "NetworkComponent.h"
#include "engine/Serializer.h"
#include "engine/Time.h"

namespace tri {

	TRI_SYSTEM(NetworkReplication);

	NetworkReplication::NetworkReplication() {
		active = false;
		hasAuthority = false;
	}

	void NetworkReplication::init() {
		env->runtimeMode->setActiveSystem<NetworkReplication>({ RuntimeMode::LOADING, RuntimeMode::EDIT, RuntimeMode::PAUSED }, true);
		env->jobManager->addJob("Network")->addSystem<NetworkReplication>();
	}

	void NetworkReplication::startup() {
		addListener = env->eventManager->onEntityAdd.addListener([&](World* world, EntityId id) {
			if (world == env->world) {
				addedRuntimeEntities.insert(EntityUtil::getGuid(id));
			}
		});
		componentAddListener = env->eventManager->onComponentAdd<NetworkComponent>().addListener([&](World* world, EntityId id) {
			if (active && hasAuthority) {
				if (auto* net = env->world->getComponent<NetworkComponent>(id)) {
					addEntity(id);
				}
			}
		});
		removeListener = env->eventManager->onEntityRemove.addListener([&](World* world, EntityId id) {
			if (world == env->world) {
				auto guid = EntityUtil::getGuid(id);
				addedRuntimeEntities.erase(guid);
				removedRuntimeEntities.insert(guid);

				if (active && hasAuthority) {
					if (auto* net = env->world->getComponent<NetworkComponent>(id)) {
						removeEntity(id);
					}
				}
			}
		});
		beginListener = env->eventManager->onMapBegin.addListener([&](World* world, std::string file) {
			removedRuntimeEntities.clear();
			addedRuntimeEntities.clear();
		});
	}

	void NetworkReplication::tick() {
		if (active && hasAuthority) {
			if (env->runtimeMode->getMode() != RuntimeMode::LOADING) {
				if (env->time->frameTicks(1.0f / 60.0f)) {
					env->world->each<NetworkComponent>([&](EntityId id, NetworkComponent& net) {
						if (net.syncAlways) {
							updateEntity(id);
						}
					});
				}
			}
		}

		if (active && !hasAuthority) {
			std::unique_lock<std::mutex> lock(operationsMutex);
			for (auto& operation : operations) {
				EntityId id = EntityUtil::getEntityByGuid(operation.guid);
				if (id != -1) {
					if (operation.opcode == NetOpcode::ENTITY_UPDATE || operation.opcode == NetOpcode::ENTITY_ADD) {
						SerialData ser;
						ser.ownigNode = YAML::Load(operation.data);
						env->serializer->deserializeEntity(id, env->world, ser);
					}
					else if (operation.opcode == NetOpcode::ENTITY_REMOVE) {
						env->world->removeEntity(id);
					}
				}
				else {
					if (operation.opcode == NetOpcode::ENTITY_ADD) {
						SerialData ser;
						ser.ownigNode = YAML::Load(operation.data);
						env->serializer->deserializeEntity(env->world, ser);
					}
				}
			}
			operations.clear();
		}
	}

	void NetworkReplication::shutdown() {
		removedRuntimeEntities.clear();
		addedRuntimeEntities.clear();
		env->eventManager->onEntityAdd.removeListener(addListener);
		env->eventManager->onComponentAdd<NetworkComponent>().removeListener(componentAddListener);
		env->eventManager->onEntityRemove.removeListener(removeListener);
		env->eventManager->onMapBegin.removeListener(beginListener);
	}

	void NetworkReplication::onData(Guid guid, NetOpcode opcode, const std::string& data) {
		TRI_PROFILE_FUNC();
		if (active && !hasAuthority) {
			std::unique_lock<std::mutex> lock(operationsMutex);
			operations.push_back({ guid, opcode, data });
		}
	}

	void NetworkReplication::updateEntity(EntityId id) {
		SerialData ser;
		ser.emitter = std::make_shared<YAML::Emitter>();
		env->serializer->serializeEntity(id, env->world, ser);
		Packet packet;
		packet.add(NetOpcode::ENTITY_UPDATE);
		packet.add(EntityUtil::getGuid(id));
		packet.addStr(ser.emitter->c_str());
		env->systemManager->getSystem<NetworkSystem>()->sendToAll(packet.data(), packet.size());
	}

	void NetworkReplication::addEntity(EntityId id) {
		SerialData ser;
		ser.emitter = std::make_shared<YAML::Emitter>();
		env->serializer->serializeEntity(id, env->world, ser);
		Packet packet;
		packet.add(NetOpcode::ENTITY_ADD);
		packet.add(EntityUtil::getGuid(id));
		packet.addStr(ser.emitter->c_str());
		env->systemManager->getSystem<NetworkSystem>()->sendToAll(packet.data(), packet.size());
	}

	void NetworkReplication::removeEntity(EntityId id) {
		SerialData ser;
		ser.emitter = std::make_shared<YAML::Emitter>();
		env->serializer->serializeEntity(id, env->world, ser);
		Packet packet;
		packet.add(NetOpcode::ENTITY_REMOVE);
		packet.add(EntityUtil::getGuid(id));
		packet.addStr("");
		env->systemManager->getSystem<NetworkSystem>()->sendToAll(packet.data(), packet.size());
	}

}
//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "NetworkReplication.h"
#include "NetworkManager.h"
#include "core/core.h"
#include "engine/RuntimeMode.h"
#include "engine/Map.h"
#include "engine/EntityUtil.h"
#include "Packet.h"
#include "NetworkComponent.h"
#include "engine/Serializer.h"
#include "engine/Time.h"

namespace tri {

	TRI_SYSTEM_INSTANCE(NetworkReplication, env->networkReplication);

	void NetworkReplication::init() {
		env->runtimeMode->setActiveSystem<NetworkReplication>({ RuntimeMode::LOADING, RuntimeMode::EDIT, RuntimeMode::PAUSED }, true);
		env->jobManager->addJob("Network")->addSystem<NetworkReplication>();
	}

	void addEntity(EntityId id, Guid guid, Connection *conn = nullptr) {
		Packet packet;
		packet.add(NetOpcode::ENTITY_ADD);
		packet.add(guid);

		SerialData ser;
		ser.emitter = std::make_shared<YAML::Emitter>();
		env->serializer->serializeEntity(id, env->world, ser);
		packet.addStr(ser.emitter->c_str());
		if (conn) {
			conn->socket->write(packet.data(), packet.size());
		}
		else {
			env->networkManager->sendToAll(packet);
		}
	}

	void updateEntity(EntityId id, Guid guid, Connection* conn = nullptr) {
		Packet packet;
		packet.add(NetOpcode::ENTITY_UPDATE);
		packet.add(guid);

		SerialData ser;
		ser.emitter = std::make_shared<YAML::Emitter>();
		env->serializer->serializeEntity(id, env->world, ser);
		packet.addStr(ser.emitter->c_str());
		if (conn) {
			conn->socket->write(packet.data(), packet.size());
		}
		else {
			env->networkManager->sendToAll(packet);
		}
	}

	void removeEntity(Guid guid, Connection *conn = nullptr) {
		Packet packet;
		packet.add(NetOpcode::ENTITY_REMOVE);
		packet.add(guid);
		if (conn) {
			conn->socket->write(packet.data(), packet.size());
		}
		else {
			env->networkManager->sendToAll(packet);
		}
	}

	void NetworkReplication::startup() {
		env->networkManager->onConnect.addListener([](Connection* conn) {
			if (env->networkManager->getMode() == NetMode::CLIENT) {
				Packet packet;
				packet.add(NetOpcode::MAP_REQUEST);
				env->networkManager->sendToAll(packet);
			}
		});

		env->networkManager->packetCallbacks[NetOpcode::MAP_REQUEST] = [](Connection* conn, NetOpcode opcode, Packet& packet) {
			Packet reply;
			reply.add(NetOpcode::MAP_RESPONSE);
			reply.addStr(env->worldFile);
			conn->socket->write(reply.data(), reply.size());
		};

		env->networkManager->packetCallbacks[NetOpcode::MAP_RESPONSE] = [](Connection* conn, NetOpcode opcode, Packet& packet) {
			if (!env->networkManager->hasAuthority()) {
				std::string file = packet.getStr();
				env->eventManager->onMapBegin.addListener([conn](World* world, std::string) {
					env->eventManager->postTick.addListener([conn]() {
						Packet reply;
						reply.add(NetOpcode::MAP_LOADED);
						conn->socket->write(reply.data(), reply.size());
					}, true);
				}, true);
				Map::loadAndSetToActiveWorld(file);
			}
		};

		env->eventManager->onMapBegin.addListener([](World* world, std::string file) {
			if (world == env->world) {
				if (env->networkManager->hasAuthority()) {
					Packet reply;
					reply.add(NetOpcode::MAP_RESPONSE);
					reply.addStr(env->worldFile);
					env->networkManager->sendToAll(reply);
				}
			}
		});

		env->networkManager->packetCallbacks[NetOpcode::MAP_LOADED] = [&](Connection* conn, NetOpcode opcode, Packet& packet) {
			if (env->networkManager->hasAuthority()) {
				for (auto &guid : removedMapEntities) {
					removeEntity(guid, conn);
				}
				for (auto& guid : addedRuntimeEntities) {
					addEntity(EntityUtil::getEntityByGuid(guid), guid, conn);
				}
			}
		};
		env->networkManager->packetCallbacks[NetOpcode::ENTITY_ADD] = [&](Connection* conn, NetOpcode opcode, Packet& packet) {
			if (!env->networkManager->hasAuthority()) {
				Guid guid = packet.get<Guid>();
				EntityId id = EntityUtil::getEntityByGuid(guid);
				if (id == -1) {
					std::unique_lock<std::mutex> lock(env->world->performePendingMutex);
					SerialData ser;
					ser.ownigNode = YAML::Load(packet.getStr());
					env->serializer->deserializeEntity(env->world, ser, &idMap);
				}
			}
		};
		env->networkManager->packetCallbacks[NetOpcode::ENTITY_UPDATE] = [&](Connection* conn, NetOpcode opcode, Packet& packet) {
			Guid guid = packet.get<Guid>();
			if (!env->networkManager->hasAuthority() || getOwningConnection(guid) == conn) {
				EntityId id = EntityUtil::getEntityByGuid(guid);
				if (id != -1) {
					std::unique_lock<std::mutex> lock(env->world->performePendingMutex);
					SerialData ser;
					ser.ownigNode = YAML::Load(packet.getStr());
					env->serializer->deserializeEntity(id, env->world, ser);
				}
				else {
					std::unique_lock<std::mutex> lock(env->world->performePendingMutex);
					SerialData ser;
					ser.ownigNode = YAML::Load(packet.getStr());
					env->serializer->deserializeEntity(env->world, ser, &idMap);
				}
			}
		};
		env->networkManager->packetCallbacks[NetOpcode::ENTITY_REMOVE] = [&](Connection* conn, NetOpcode opcode, Packet& packet) {
			if (!env->networkManager->hasAuthority()) {
				std::unique_lock<std::mutex> lock(env->world->performePendingMutex);
				Guid guid = packet.get<Guid>();
				EntityId id = EntityUtil::getEntityByGuid(guid);
				env->world->removeEntity(id);
			}
		};
		env->networkManager->packetCallbacks[NetOpcode::ENTITY_OWNING] = [&](Connection* conn, NetOpcode opcode, Packet& packet) {
			if (!env->networkManager->hasAuthority()) {
				Guid guid = packet.get<Guid>();
				owning.insert(guid);
			}
		};

		env->eventManager->onMapBegin.addListener([&](World* world, std::string file) {
			if (world == env->world) {
				mapEntities.clear();
				addedRuntimeEntities.clear();
				removedRuntimeEntities.clear();
				world->each([&](EntityId id) {
					mapEntities.insert(EntityUtil::getGuid(id));
				});
			}
		});
		env->eventManager->onEntityAdd.addListener([&](World* world, EntityId id) {
			if (world == env->world) {
				if (auto* net = env->world->getComponent<NetworkComponent>(id)) {
					Guid guid = EntityUtil::getGuid(id);
					addedRuntimeEntities.insert(guid);
				}
			}
		});
		env->eventManager->onComponentAdd<NetworkComponent>().addListener([&](World* world, EntityId id) {
			if (world == env->world) {
				Guid guid = EntityUtil::getGuid(id);
				addedRuntimeEntities.insert(guid);
				if (env->networkManager->hasAuthority()) {
					if (env->runtimeMode->getMode() != RuntimeMode::LOADING) {
						addEntity(id, guid);
					}
				}
			}
		});
		env->eventManager->onEntityRemove.addListener([&](World* world, EntityId id) {
			if (world == env->world) {
				Guid guid = EntityUtil::getGuid(id);
				if (mapEntities.contains(guid)) {
					removedMapEntities.insert(guid);
					if (env->networkManager->hasAuthority()) {
						removeEntity(guid);
					}
				}
				else {
					addedRuntimeEntities.erase(guid);
					removedRuntimeEntities.insert(guid);
					if (env->networkManager->hasAuthority()) {
						if (auto* net = env->world->getComponent<NetworkComponent>(id)) {
							removeEntity(guid);
						}
					}
				}
			}
		});

		env->networkManager->onDisconnect.addListener([&](Connection* conn) {
			for (auto& i : owningConnections) {
				if (i.second == conn) {
					env->world->removeEntity(EntityUtil::getEntityByGuid(i.first));
				}
			}
		});

		EntityUtil::setIsEntityOwningFunction([&](Guid guid) {
			return isOwning(guid);
		});
	}

	void NetworkReplication::tick() {
		if (env->time->frameTicks(1.0f / 60.0f)) {
			env->world->each<NetworkComponent>([&](EntityId id, NetworkComponent& net) {
				if (net.syncAlways) {
					Guid guid = EntityUtil::getGuid(id);
					if (isOwning(guid)) {
						updateEntity(id, EntityUtil::getGuid(id));
					}
				}
			});
		}
		if (!idMap.empty()) {
			EntityUtil::replaceIds(idMap, env->world);
			idMap.clear();
		}
	}

	void NetworkReplication::shutdown() {
		EntityUtil::setIsEntityOwningFunction(nullptr);
	}

	void NetworkReplication::setOwning(Guid guid, Connection *conn) {
		if (env->networkManager->hasAuthority()) {
			if (conn) {
				owningConnections[guid] = conn;

				Packet packet;
				packet.add(NetOpcode::ENTITY_OWNING);
				packet.add(guid);
				conn->socket->write(packet.data(), packet.size());
			}
			else {
				owningConnections.erase(guid);
				//todo: unown on client
			}
		}
	}

	bool NetworkReplication::isOwning(Guid guid) {
		if (env->networkManager->hasAuthority()) {
			return owningConnections.find(guid) == owningConnections.end();
		}
		else {
			return owning.contains(guid);
		}
	}

	Connection* NetworkReplication::getOwningConnection(Guid guid) {
		auto entry = owningConnections.find(guid);
		if (entry == owningConnections.end()) {
			return nullptr;
		}
		return entry->second;
	}

}
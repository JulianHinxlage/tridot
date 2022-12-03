//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "NetworkReplication.h"
#include "NetworkManager.h"
#include "core/core.h"
#include "engine/RuntimeMode.h"
#include "engine/Map.h"
#include "engine/EntityUtil.h"
#include "NetworkComponent.h"
#include "engine/Serializer.h"
#include "engine/Time.h"

namespace tri {

	TRI_SYSTEM_INSTANCE(NetworkReplication, env->networkReplication);

	void NetworkReplication::init() {
		env->runtimeMode->setActiveSystem<NetworkReplication>({ RuntimeMode::LOADING, RuntimeMode::EDIT, RuntimeMode::PAUSED }, true);
		env->jobManager->addJob("Network")->addSystem<NetworkReplication>();
		enableClientSideEntitySpawning = true;
		enableClientSideEntityDepawning = false;
	}

	void NetworkReplication::startup() {
		env->networkManager->onConnect.addListener([](Connection* conn) {
			if (env->networkManager->getMode() == NetMode::CLIENT) {
				Packet packet;
				packet.writeBin(NetOpcode::MAP_REQUEST);
				env->networkManager->sendToAll(packet);
			}
		});

		env->networkManager->packetCallbacks[NetOpcode::MAP_REQUEST] = [](Connection* conn, NetOpcode opcode, Packet& packet) {
			Packet reply;
			reply.writeBin(NetOpcode::MAP_RESPONSE);
			reply.writeStr(env->worldFile);
			conn->write(reply.data(), reply.size());
		};

		env->networkManager->packetCallbacks[NetOpcode::MAP_RESPONSE] = [](Connection* conn, NetOpcode opcode, Packet& packet) {
			if (!env->networkManager->hasAuthority()) {
				std::string file = packet.readStr();
				env->eventManager->onMapBegin.addListener([conn](World* world, std::string) {
					
					//delay by one frame to ensure entity guid to id mapping is updated
					env->eventManager->preTick.addListener([conn]() {
						env->eventManager->postTick.addListener([conn]() {
							Packet reply;
							reply.writeBin(NetOpcode::MAP_LOADED);
							conn->write(reply.data(), reply.size());
						}, true);
					}, true);
					
				}, true);
				Map::loadAndSetToActiveWorld(file, RuntimeMode::PAUSED);
			}
		};

		env->networkManager->packetCallbacks[NetOpcode::MAP_LOADED] = [&](Connection* conn, NetOpcode opcode, Packet& packet) {
			if (env->networkManager->hasAuthority()) {
				for (auto &guid : removedMapEntities) {
					removeEntity(guid, conn);
				}

				for (auto& guid : addedRuntimeEntities) {
					addEntity(EntityUtil::getEntityByGuid(guid), guid, conn);
				}

				env->world->each<NetworkComponent>([&](EntityId id, NetworkComponent& net) {
					Guid guid = EntityUtil::getGuid(id);
					updateEntity(id, guid, conn);
				});

				Packet reply;
				reply.writeBin(NetOpcode::MAP_SYNCED);
				conn->write(reply.data(), reply.size());
			}
		};
		env->networkManager->packetCallbacks[NetOpcode::MAP_SYNCED] = [&](Connection* conn, NetOpcode opcode, Packet& packet) {
			env->runtimeMode->setMode(RuntimeMode::PLAY);
			Packet reply;
			reply.writeBin(NetOpcode::MAP_JOIN);
			conn->write(reply.data(), reply.size());
		};

		env->networkManager->packetCallbacks[NetOpcode::ENTITY_ADD] = [&](Connection* conn, NetOpcode opcode, Packet& packet) {
			if (enableClientSideEntitySpawning || !env->networkManager->hasAuthority()) {
				Guid guid = packet.readBin<Guid>();
				EntityId id = EntityUtil::getEntityByGuid(guid);
				if (id == -1) {
					addedNetworkEntities.insert(guid);
					std::unique_lock<std::mutex> lock(env->world->performePendingMutex);
					env->serializer->deserializeEntityBinary(env->world, packet, &idMap);
				}
				else {
					env->console->log(LogLevel::TRACE, "Network", "entity %s already exists", guid.toString().c_str());
				}
				if (env->networkManager->hasAuthority()) {
					packet.reset();
					env->networkManager->sendToAll(packet, conn);
				}
			}
		};
		env->networkManager->packetCallbacks[NetOpcode::ENTITY_UPDATE] = [&](Connection* conn, NetOpcode opcode, Packet& packet) {
			Guid guid = packet.readBin<Guid>();
			if (!env->networkManager->hasAuthority() || getOwningConnection(guid) == conn) {
				EntityId id = EntityUtil::getEntityByGuid(guid);
				if (id != -1) {
					std::unique_lock<std::mutex> lock(env->world->performePendingMutex);
					env->serializer->deserializeEntityBinary(id, env->world, packet, &idMap);
				}
				else {
					env->console->log(LogLevel::TRACE, "Network", "entity %s dose not exists", guid.toString().c_str());
				}
				if (env->networkManager->hasAuthority()) {
					packet.reset();
					env->networkManager->sendToAll(packet, conn);
				}
			}
		};
		env->networkManager->packetCallbacks[NetOpcode::ENTITY_REMOVE] = [&](Connection* conn, NetOpcode opcode, Packet& packet) {
			if (enableClientSideEntityDepawning || !env->networkManager->hasAuthority()) {
				std::unique_lock<std::mutex> lock(env->world->performePendingMutex);
				Guid guid = packet.readBin<Guid>();
				removedNetworkEntities.insert(guid);
				EntityId id = EntityUtil::getEntityByGuid(guid);
				env->world->removeEntity(id);
			}
		};
		env->networkManager->packetCallbacks[NetOpcode::ENTITY_OWNING] = [&](Connection* conn, NetOpcode opcode, Packet& packet) {
			if (!env->networkManager->hasAuthority()) {
				Guid guid = packet.readBin<Guid>();
				owning.insert(guid);
			}
		};

		env->eventManager->onMapBegin.addListener([&](World* world, std::string file) {
			if (world == env->world) {
				mapEntities.clear();
				removedMapEntities.clear();
				addedRuntimeEntities.clear();
				removedRuntimeEntities.clear();
				addedNetworkEntities.clear();
				removedNetworkEntities.clear();
				world->each([&](EntityId id) {
					mapEntities.insert(EntityUtil::getGuid(id));
				});

				if (env->networkManager->hasAuthority()) {
					for (auto &conn : env->networkManager->getConnections()) {
						conn->clientState = Connection::CONNECTED;
					}

					Packet reply;
					reply.writeBin(NetOpcode::MAP_RESPONSE);
					reply.writeStr(env->worldFile);
					env->networkManager->sendToAll(reply);
				}
			}
		});
		env->eventManager->onComponentAdd<NetworkComponent>().addListener([&](World* world, EntityId id) {
			if (world == env->world) {
				Guid guid = EntityUtil::getGuid(id);
				addedRuntimeEntities.insert(guid);
				if (enableClientSideEntitySpawning || env->networkManager->hasAuthority()) {
					if (!addedNetworkEntities.contains(guid)) {
						addEntity(id, guid);
					}
				}
				else {
					if (!addedNetworkEntities.contains(guid)) {
						owning.insert(guid);
					}
				}
			}
		});
		env->eventManager->onEntityRemove.addListener([&](World* world, EntityId id) {
			if (world == env->world) {
				Guid guid = EntityUtil::getGuid(id);

				if (!env->networkManager->hasAuthority()) {
					if (!env->networkReplication->isOwning(guid)) {
						if (!enableClientSideEntityDepawning) {
							if (!removedNetworkEntities.contains(guid)) {
								if (auto* net = env->world->getComponent<NetworkComponent>(id)) {
									env->world->preventPendingEntityRemove(id);
									return;
								}
							}
						}
					}
				}

				if (mapEntities.contains(guid)) {
					removedMapEntities.insert(guid);
					if (enableClientSideEntityDepawning || env->networkManager->hasAuthority()) {
						removeEntity(guid);
					}
				}
				else {
					addedRuntimeEntities.erase(guid);
					removedRuntimeEntities.insert(guid);
					if (enableClientSideEntityDepawning || env->networkManager->hasAuthority()) {
						if (auto* net = env->world->getComponent<NetworkComponent>(id)) {
							removeEntity(guid);
						}
					}
				}
			}
		});

		env->networkManager->onDisconnect.addListener([&](Connection* conn) {
			std::unique_lock<std::mutex> lock(env->world->performePendingMutex);
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
				packet.writeBin(NetOpcode::ENTITY_OWNING);
				packet.writeBin(guid);
				conn->write(packet.data(), packet.size());
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

	void NetworkReplication::addEntity(EntityId id, Guid guid, Connection* conn) {
		Packet packet;
		packet.writeBin(NetOpcode::ENTITY_ADD);
		packet.writeBin(guid);

		env->serializer->serializeEntityBinary(id, env->world, packet);
		if (conn) {
			conn->write(packet.data(), packet.size());
		}
		else {
			env->networkManager->sendToAll(packet);
		}
	}

	void NetworkReplication::updateEntity(EntityId id, Guid guid, Connection* conn) {
		Packet packet;
		packet.writeBin(NetOpcode::ENTITY_UPDATE);
		packet.writeBin(guid);

		env->serializer->serializeEntityBinary(id, env->world, packet);
		if (conn) {
			conn->write(packet.data(), packet.size());
		}
		else {
			env->networkManager->sendToAll(packet);
		}
	}

	void NetworkReplication::removeEntity(Guid guid, Connection* conn) {
		Packet packet;
		packet.writeBin(NetOpcode::ENTITY_REMOVE);
		packet.writeBin(guid);
		if (conn) {
			conn->write(packet.data(), packet.size());
		}
		else {
			env->networkManager->sendToAll(packet);
		}
	}

}
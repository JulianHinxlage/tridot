//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "NetworkSystem.h"
#include "core/core.h"
#include "engine/RuntimeMode.h"
#include "engine/Map.h"
#include "Packet.h"
#include "NetworkReplication.h"

#if TRI_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<arpa/inet.h>
#endif

namespace tri {
	
	TRI_SYSTEM(NetworkSystem);

	void NetworkSystem::init() {
		env->runtimeMode->setActiveSystem<NetworkSystem>({ RuntimeMode::LOADING, RuntimeMode::EDIT, RuntimeMode::PAUSED }, true);

		mode = STANDALONE;
		strMode = "";
		serverAddress = "localhost";
		serverPort = 24052;

		env->console->addCVar("serverAdress", &serverAddress);
		env->console->addCVar("serverPort", &serverPort);
		env->console->addCVar<std::string>("networkMode", "standalone");
	}

	void NetworkSystem::startup() {
#if TRI_WINDOWS
		WSADATA wsaData;
		int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (result != NO_ERROR) {
			env->console->error("WSA startup failed with code %i", result);
			return;
		}
#endif

		env->eventManager->onMapBegin.addListener([&](World* world, std::string file) {
			if (mode == SERVER || mode == HOST) {
				Packet reply;
				reply.add(NetOpcode::LOAD_MAP);
				reply.addStr(env->worldFile);
				for (auto& conn : connections) {
					conn->socket->write(reply.data(), reply.size());
				}
			}
		});
	}

	void NetworkSystem::tick() {
		std::string strMode = env->console->getCVarValue<std::string>("networkMode", "standalone");
		strMode = StrUtil::toLower(strMode);
		if (strMode != this->strMode) {
			if (strMode == "standalone") {
				this->strMode = strMode;
				setMode(STANDALONE);
			}
			else if (strMode == "client") {
				this->strMode = strMode;
				setMode(CLIENT);
			}
			else if (strMode == "server") {
				this->strMode = strMode;
				setMode(SERVER);
			}
			else if (strMode == "host") {
				this->strMode = strMode;
				setMode(HOST);
			}
		}

		if (mode == CLIENT) {
			if (tryReconnect) {
				tryReconnect = false;
				setMode(CLIENT);
			}
		}

		disconnectedConnections.clear();
	}

	void NetworkSystem::shutdown() {
		connection->stop();
		for (auto& conn : connections) {
			conn->stop();
		}
		connections.clear();
		disconnectedConnections.clear();

#if TRI_WINDOWS
		WSACleanup();
#endif
	}

	void NetworkSystem::setMode(Mode mode) {
		this->mode = mode;

		if (mode == STANDALONE) {
			if (connection) {
				connection->stop();
				connection = nullptr;
				connections.clear();
			}

			auto* replication = env->systemManager->getSystem<NetworkReplication>();
			replication->active = false;
			replication->hasAuthority = false;
		}
		else if (mode == CLIENT) {
			if (!connection) {
				connection = Ref<Connection>::make();
			}
			else {
				connection->stop();
			}

			connection->onConnect = [&](Connection* conn) {
				env->console->info("connected to %s %i", conn->socket->getEndpoint().getAddress().c_str(), conn->socket->getEndpoint().getPort());
				auto* replication = env->systemManager->getSystem<NetworkReplication>();
				replication->active = true;
				replication->hasAuthority = false;
				onConnect(conn);
			};
			connection->onDisconnect = [&](Connection* conn) {
				env->console->info("disconnected from %s %i", conn->socket->getEndpoint().getAddress().c_str(), conn->socket->getEndpoint().getPort());
				onDisconnect(conn);
				tryReconnect = true;
			};
			connection->onFail = [&](Connection* conn) {
				env->console->error("failed to connect to %s %i", serverAddress.c_str(), serverPort);
				tryReconnect = true;
			};
			connection->runConnect(serverAddress, serverPort, [&](Connection* conn, void* data, int bytes) {
				onRead(conn, data, bytes);
			});
		}
		else if (mode == SERVER || mode == HOST) {
			if (!connection) {
				connection = Ref<Connection>::make();
			}
			else {
				connection->stop();
			}

			connection->onFail = [&](Connection* conn) {
				env->console->error("failed to listen on port %i", serverPort);
			};
			connection->onConnect = [&](Connection* conn) {
				env->console->info("listen on port %i", serverPort);
				auto *replication = env->systemManager->getSystem<NetworkReplication>();
				replication->active = true;
				replication->hasAuthority = true;
			};
			connection->runListen(serverPort, [&](Ref<Connection> conn) {
				connections.push_back(conn);
				conn->onDisconnect = [&](Connection* conn) {
					env->console->info("disconnect from %s %i", conn->socket->getEndpoint().getAddress().c_str(), conn->socket->getEndpoint().getPort());
					onDisconnect(conn);
					for (int i = 0; i < connections.size(); i++) {
						if (connections[i].get() == conn) {
							disconnectedConnections.push_back(connections[i]);
							connections.erase(connections.begin() + i);
							break;
						}
					}
				};
				env->console->info("connection from %s %i", conn->socket->getEndpoint().getAddress().c_str(), conn->socket->getEndpoint().getPort());
				onConnect(conn.get());
				conn->run([&](Connection* conn, void* data, int bytes) {
					onRead(conn, data, bytes);
				});
			});
		}
	}

	void NetworkSystem::sendToAll(void* data, int bytes) {
		if (mode == CLIENT) {
			connection->socket->write(data, bytes);
		}
		else if (mode == SERVER || mode == HOST) {
			for (auto& conn : connections) {
				conn->socket->write(data, bytes);
			}
		}
	}

	void NetworkSystem::onRead(Connection* conn, void* data, int bytes) {
		Packet packet;
		packet.add(data, bytes);

		NetOpcode opcode = packet.get<NetOpcode>();

		switch (opcode) {
		case NOOP:
			break;
		case JOIN: {
			Packet reply;
			reply.add(NetOpcode::LOAD_MAP);
			reply.addStr(env->worldFile);
			conn->socket->write(reply.data(), reply.size());
			break;
		}
		case LOAD_MAP: {
			if (mode == CLIENT) {
				std::string file = packet.getStr();
				Map::loadAndSetToActiveWorld(file);
			}
			break;
		}
		case ENTITY_ADD:
		case ENTITY_UPDATE:
		case ENTITY_REMOVE:{
			Guid guid = packet.get<Guid>();
			env->systemManager->getSystem<NetworkReplication>()->onData(guid, opcode, packet.getStr());
			break;
		}
		default:
			env->console->warning("invalid opcode %i", (int)opcode);
			break;
		}
	}

	void NetworkSystem::onConnect(Connection* conn) {
		if (mode == CLIENT) {
			Packet packet;
			packet.add(NetOpcode::JOIN);
			conn->socket->write(packet.data(), packet.size());
		}
	}

	void NetworkSystem::onDisconnect(Connection* conn) {

	}

}

#include "NetworkManager.h"
#include "NetworkManager.h"
//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "NetworkManager.h"
#include "core/core.h"
#include "engine/RuntimeMode.h"
#include "Packet.h"
#include "NetworkReplication.h"
#include "engine/EntityUtil.h"
#include "NetworkComponent.h"
#include "engine/Time.h"

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
	
	TRI_SYSTEM_INSTANCE(NetworkManager, env->networkManager);

	TRI_CLASS(NetOpcode);
	TRI_ENUM8(NetOpcode, NOOP, MAP_REQUEST, MAP_RESPONSE, MAP_LOADED, MAP_SYNCED, MAP_JOIN, ENTITY_ADD, ENTITY_UPDATE);
	TRI_ENUM3(NetOpcode, ENTITY_REMOVE, ENTITY_OWNING, PROPERTY_DATA);

	void NetworkManager::init() {
		env->runtimeMode->setActiveSystem<NetworkManager>({ RuntimeMode::LOADING, RuntimeMode::EDIT, RuntimeMode::PAUSED }, true);
		env->jobManager->addJob("Network")->addSystem<NetworkManager>();

		mode = STANDALONE;
		strMode = "";
		serverAddress = "localhost";
		serverPort = 24052;

		env->console->addCVar("serverAdress", &serverAddress);
		env->console->addCVar("serverPort", &serverPort);
		env->console->addCVar<std::string>("networkMode", "standalone");
	}

	void NetworkManager::startup() {
#if TRI_WINDOWS
		WSADATA wsaData;
		int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (result != NO_ERROR) {
			env->console->error("WSA startup failed with code %i", result);
			return;
		}
#endif

		env->console->addCommand("networkStats", [&](auto args) {
			env->console->info("network statistics:");

			if (hasAuthority()) {
				if (connections.size() == 1) {
					env->console->info("%i connection", connections.size());
				}
				else {
					env->console->info("%i connections", connections.size());
				}
			}
			else {
				if (connection && connection->socket->isConnected()) {
					env->console->info("connected to server");
				}
				else {
					env->console->info("not connected to server");
				}
			}

			env->console->info("%i k/sec up", bytesUpPerSecond / 1000);
			env->console->info("%i k/sec down", bytesDownPerSecond / 1000);
		});
	}

	void NetworkManager::tick() {
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
				setMode(CLIENT);
				tryReconnect = false;
			}
		}

		disconnectedConnections.clear();

		if (env->time->frameTicks(1.0f)) {
			bytesDownPerSecond = 0;
			bytesUpPerSecond = 0;

			if (connection) {
				bytesDownPerSecond += connection->socket->bytesDown;
				bytesUpPerSecond += connection->socket->bytesUp;
				connection->socket->bytesDown = 0;
				connection->socket->bytesUp = 0;
			}
			for (auto& conn : connections) {
				if (conn) {
					bytesDownPerSecond += conn->socket->bytesDown;
					bytesUpPerSecond += conn->socket->bytesUp;
					conn->socket->bytesDown = 0;
					conn->socket->bytesUp = 0;
				}
			}
		}
	}

	void NetworkManager::shutdown() {
		if (connection) {
			connection->stop();
		}
		for (auto& conn : connections) {
			conn->stop();
		}
		connections.clear();
		disconnectedConnections.clear();
		packetCallbacks.clear();

		env->console->removeCommand("networkStats");

#if TRI_WINDOWS
		WSACleanup();
#endif
	}

	void NetworkManager::setMode(NetMode mode) {
		this->mode = mode;

		if (mode == STANDALONE) {
			if (connection) {
				connection->stop();
				connection = nullptr;
				connections.clear();
			}
		}
		else if (mode == CLIENT) {
			if (!connection) {
				connection = Ref<Connection>::make();
			}
			else {
				if (!tryReconnect) {
					connection->stop();
				}
			}

			connection->onConnect = [&](Connection* conn) {
				failWarning = true;
				env->console->info("connected to %s %i", conn->socket->getEndpoint().getAddress().c_str(), conn->socket->getEndpoint().getPort());
				onConnect.invoke(conn);
			};
			connection->onDisconnect = [&](Connection* conn) {
				env->console->info("disconnected from %s %i", conn->socket->getEndpoint().getAddress().c_str(), conn->socket->getEndpoint().getPort());
				onDisconnect.invoke(conn);
				tryReconnect = true;
			};
			connection->onFail = [&](Connection* conn) {
				if (failWarning) {
					env->console->error("failed to connect to %s %i", serverAddress.c_str(), serverPort);
					failWarning = false;
				}
				tryReconnect = true;
			};
			if (tryReconnect) {
				connection->reconnect.notify_one();
			}
			else {
				connection->runConnect(serverAddress, serverPort, [&](Connection* conn, void* data, int bytes) {
					onRead(conn, data, bytes);
				});
			}
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
			};
			connection->runListen(serverPort, [&](Ref<Connection> conn) {
				connections.push_back(conn);
				conn->onDisconnect = [&](Connection* conn) {
					env->console->info("disconnect from %s %i", conn->socket->getEndpoint().getAddress().c_str(), conn->socket->getEndpoint().getPort());
					onDisconnect.invoke(conn);
					for (int i = 0; i < connections.size(); i++) {
						if (connections[i].get() == conn) {
							disconnectedConnections.push_back(connections[i]);
							connections.erase(connections.begin() + i);
							break;
						}
					}
				};
				env->console->info("connection from %s %i", conn->socket->getEndpoint().getAddress().c_str(), conn->socket->getEndpoint().getPort());
				onConnect.invoke(conn.get());
				conn->run([&](Connection* conn, void* data, int bytes) {
					onRead(conn, data, bytes);
				});
			});
		}
	}

	bool NetworkManager::isConnected() {
		if (mode == CLIENT) {
			return connection->socket->isConnected();
		}
		else if (mode == SERVER || mode == HOST) {
			return connections.size() > 0;
		}
		return false;
	}

	bool NetworkManager::hasAuthority() {
		return mode != CLIENT;
	}

	void NetworkManager::sendToAll(const void* data, int bytes, Connection* except) {
		if (mode == CLIENT) {
			connection->write(data, bytes);
		}
		else if (mode == SERVER || mode == HOST) {
			for (auto& conn : connections) {
				if (conn.get() != except) {
					conn->write(data, bytes);
				}
			}
		}
	}

	void NetworkManager::sendToAll(Packet& packet, Connection* except) {
		sendToAll(packet.data(), packet.size(), except);
	}

	void NetworkManager::onRead(Connection* conn, void* data, int bytes) {
		TRI_PROFILE_FUNC();
		Packet packet;
		packet.add(data, bytes);
		NetOpcode opcode = packet.get<NetOpcode>();

		auto entry = packetCallbacks.find(opcode);
		if (entry != packetCallbacks.end()) {
			entry->second(conn, opcode, packet);
		}
		else {
			if (opcode != NOOP) {
				env->console->warning("invalid opcode %i", opcode);
			}
		}
	}

}

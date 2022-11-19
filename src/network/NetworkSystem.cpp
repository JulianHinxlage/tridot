//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "NetworkSystem.h"
#include "core/core.h"
#include "engine/RuntimeMode.h"

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

		tick();
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

		disconnectedConnections.clear();
	}

	void NetworkSystem::shutdown() {
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
				onConnect(conn);
			};
			connection->onDisconnect = [&](Connection* conn) {
				env->console->info("disconnected from %s %i", conn->socket->getEndpoint().getAddress().c_str(), conn->socket->getEndpoint().getPort());
				onDisconnect(conn);
			};
			connection->onFail = [&](Connection* conn) {
				env->console->error("failed to connect to %s %i", serverAddress.c_str(), serverPort);
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

	void NetworkSystem::onRead(Connection* conn, void* data, int bytes) {

	}

	void NetworkSystem::onConnect(Connection* conn) {
	
	}

	void NetworkSystem::onDisconnect(Connection* conn) {

	}

}

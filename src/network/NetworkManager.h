//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/System.h"
#include "Connection.h"
#include "engine/EntityEvent.h"
#include "engine/Archive.h"

namespace tri {

	enum NetOpcode {
		NOOP,
		MAP_REQUEST,
		MAP_RESPONSE,
		MAP_LOADED,
		MAP_SYNCED,
		MAP_JOIN,
		ENTITY_ADD,
		ENTITY_UPDATE,
		ENTITY_REMOVE,
		ENTITY_OWNING,
		PROPERTY_DATA,
	};

	enum NetMode {
		STANDALONE,
		CLIENT,
		SERVER,
		HOST,
	};

	class NetworkManager : public System {
	public:
		int bytesUpPerSecond = 0;
		int bytesDownPerSecond = 0;

		void init() override;
		void startup() override;
		void tick() override;
		void shutdown() override;

		void setMode(NetMode mode);
		NetMode getMode() { return mode; }

		bool isConnected();
		bool hasAuthority();
		void sendToAll(const void* data, int bytes, Connection* except = nullptr);
		void sendToAll(Packet& packet, Connection* except = nullptr);
		std::vector<Ref<Connection>>& getConnections();
		Ref<Connection> &getConnection();

		Event<Connection*> onConnect;
		Event<Connection*> onDisconnect;

		std::map<NetOpcode, std::function<void(Connection *conn, NetOpcode opcode, Packet &packet)>> packetCallbacks;

	private:
		NetMode mode;
		std::string strMode;
		int serverPort;
		std::string serverAddress;
		bool tryReconnect = false;
		bool failWarning = true;

		Ref<Connection> connection;
		std::vector<Ref<Connection>> connections;
		std::vector<Ref<Connection>> disconnectedConnections;

		void onRead(Connection* conn, void* data, int bytes);
	};

}

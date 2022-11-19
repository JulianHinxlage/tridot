//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/System.h"
#include "Connection.h"

namespace tri {

	enum NetOpcode {
		NOOP,
		JOIN,
		LOAD_MAP,
		ENTITY_ADD,
		ENTITY_UPDATE,
		ENTITY_REMOVE,
	};

	class NetworkSystem : public System {
	public:
		enum Mode {
			STANDALONE,
			CLIENT,
			SERVER,
			HOST,
		};
		
		void init();
		void startup() override;
		void tick() override;
		void shutdown() override;

		void setMode(Mode mode);
		Mode getMode() { return mode; }

		void sendToAll(void* data, int bytes);

	private:
		Mode mode;
		std::string strMode;
		int serverPort;
		std::string serverAddress;
		bool tryReconnect = false;

		Ref<Connection> connection;
		std::vector<Ref<Connection>> connections;
		std::vector<Ref<Connection>> disconnectedConnections;

		void onRead(Connection* conn, void* data, int bytes);
		void onConnect(Connection* conn);
		void onDisconnect(Connection* conn);
	};

}

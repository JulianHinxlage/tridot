//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/System.h"
#include "Connection.h"

namespace tri {

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

	private:
		Mode mode;
		std::string strMode;
		int serverPort;
		std::string serverAddress;

		Ref<Connection> connection;
		std::vector<Ref<Connection>> connections;
		std::vector<Ref<Connection>> disconnectedConnections;

		void onRead(Connection* conn, void* data, int bytes);
		void onConnect(Connection* conn);
		void onDisconnect(Connection* conn);
	};

}

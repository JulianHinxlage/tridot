//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "TcpSocket.h"
#include "engine/Archive.h"

namespace tri {

	typedef MemoryArchive Packet;

	class Connection {
	public:
		std::vector<uint8_t> buffer;
		Ref<TcpSocket> socket;
		int threadId;
		std::mutex reconnectMutex;
		std::mutex writeMutex;
		std::condition_variable reconnect;
		bool running;
		StringArchive readStringArchive;
		StringArchive writeStringArchive;
		std::function<void(Connection* conn)> onDisconnect;
		std::function<void(Connection* conn)> onConnect;
		std::function<void(Connection* conn)> onFail;

		enum State {
			NOT_CONNECTED,
			CONNECTED,
			JOINED,
		};
		State clientState = NOT_CONNECTED;

		Connection();
		~Connection();
		void run(const std::function<void(Connection* conn, void* data, int bytes)>& callback);
		void runConnect(const std::string &address, uint16_t port, const std::function<void(Connection* conn, void* data, int bytes)>& callback);
		void runListen(uint16_t port, const std::function<void(Ref<Connection> conn)>& callback);
		void stop();

		bool write(const void* data, int bytes);
		bool write(Packet &packet);
	private:
		void runImpl(const std::function<void(Connection* conn, void* data, int bytes)>& callback);
	};

}

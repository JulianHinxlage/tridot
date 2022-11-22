//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "core/core.h"
#include "Connection.h"

namespace tri {

	Connection::Connection() {
		threadId = -1;
		socket = Ref<TcpSocket>::make();
		onDisconnect = nullptr;
		onConnect = nullptr;
		onFail = nullptr;
	}

	Connection::~Connection() {
		stop();
	}

	void Connection::run(const std::function<void(Connection* conn, void* data, int bytes)>& callback) {
		if (threadId != -1) {
			env->threadManager->terminateThread(threadId);
			threadId = -1;
		}
		threadId = env->threadManager->addThread("Connection", [&, callback]() {
			runImpl(callback);
			if (onDisconnect) {
				onDisconnect(this);
			}
		});
	}

	void Connection::runConnect(const std::string& address, uint16_t port, const std::function<void(Connection* conn, void* data, int bytes)>& callback) {
		if (threadId != -1) {
			env->threadManager->terminateThread(threadId);
			threadId = -1;
		}
		running = true;
		threadId = env->threadManager->addThread("Connection", [&, address, port, callback]() {

			while (running) {
				if (socket->connect(address, port)) {
					if (onConnect) {
						onConnect(this);
					}
				}
				else {
					if (onFail) {
						onFail(this);
					}
					std::unique_lock<std::mutex> lock(reconnectMutex);
					reconnect.wait(lock);
					std::this_thread::sleep_for(std::chrono::seconds(1));
					continue;
				}

				runImpl(callback);
				if (onDisconnect) {
					onDisconnect(this);
				}

				std::unique_lock<std::mutex> lock(reconnectMutex);
				reconnect.wait(lock);
			}

		});
	}

	void Connection::runListen(uint16_t port, const std::function<void(Ref<Connection>conn)>& callback) {
		if (threadId != -1) {
			env->threadManager->terminateThread(threadId);
			threadId = -1;
		}
		running = true;
		threadId = env->threadManager->addThread("Listen", [&, port, callback]() {
			if (socket->listen(port)) {
				if (onConnect) {
					onConnect(this);
				}
			}
			else{
				if (onFail) {
					onFail(this);
				}
				return;
			}
			
			while (running && socket->isConnected()) {
				Ref<TcpSocket> newSocket = socket->accept();
				if (newSocket) {
					Ref<Connection> conn = Ref<Connection>::make();
					conn->socket = newSocket;
					callback(conn);
				}
			}
			if (onDisconnect) {
				onDisconnect(this);
			}
		});
	}

	void Connection::stop() {
		running = false;
		reconnect.notify_all();
		socket->disconnect();
		env->threadManager->terminateThread(threadId);
		threadId = -1;
	}

	bool Connection::write(const void* data, int bytes) {
		socket->write(&bytes, sizeof(bytes));
		return socket->write(data, bytes);
	}

	bool Connection::write(Packet& packet) {
		return write(packet.data(), packet.size());
	}

	void Connection::runImpl(const std::function<void(Connection* conn, void* data, int bytes)>& callback) {
		while (socket->isConnected()) {
			int packetSize = 0;
			int packetSizeSize = sizeof(packetSize);
			if (socket->read(&packetSize, packetSizeSize)) {
				if (buffer.size() < packetSize) {
					buffer.resize(packetSize);
				}
				int index = 0;
				while (index < packetSize) {
					int bytes = packetSize - index;
					if (socket->read(buffer.data() + index, bytes)) {
						index += bytes;
					}
				}
				callback(this, buffer.data(), packetSize);
			}
		}
	}

}

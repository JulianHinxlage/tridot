//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "pch.h"
#include "Connection.h"
#include "core/core.h"

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
		buffer.resize(1024);
		if (threadId != -1) {
			env->threadManager->terminateThread(threadId);
			threadId = -1;
		}
		threadId = env->threadManager->addThread("Connection", [&, callback]() {
			while (socket->isConnected()) {
				int bytes = buffer.size();
				if (socket->read(buffer.data(), bytes)) {
					callback(this, buffer.data(), bytes);
				}
			}
			if (onDisconnect) {
				onDisconnect(this);
			}
		});
	}

	void Connection::runConnect(const std::string& address, uint16_t port, const std::function<void(Connection* conn, void* data, int bytes)>& callback) {
		buffer.resize(1024);
		if (threadId != -1) {
			env->threadManager->terminateThread(threadId);
			threadId = -1;
		}
		threadId = env->threadManager->addThread("Connection", [&, address, port, callback]() {
			if (socket->connect(address, port)) {
				if (onConnect) {
					onConnect(this);
				}
			}
			else {
				if (onFail) {
					onFail(this);
				}
			}

			while (socket->isConnected()) {
				int bytes = buffer.size();
				if (socket->read(buffer.data(), bytes)) {
					callback(this, buffer.data(), bytes);
				}
			}
			if (onDisconnect) {
				onDisconnect(this);
			}
			});
	}

	void Connection::runListen(uint16_t port, const std::function<void(Ref<Connection>conn)>& callback) {
		if (threadId != -1) {
			env->threadManager->terminateThread(threadId);
			threadId = -1;
		}
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
			}
			
			while (socket->isConnected()) {
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
		socket->disconnect();
		env->threadManager->joinThread(threadId);
		env->threadManager->terminateThread(threadId);
		threadId = -1;
	}

}

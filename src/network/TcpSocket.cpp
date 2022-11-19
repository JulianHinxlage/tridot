//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "TcpSocket.h"
#include "core/config.h"

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

#include <iostream>

void printSocketError() {
#if TRI_DEBUG
	wchar_t* s = NULL;
	int error = WSAGetLastError();
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&s, 0, NULL);
	printf("socket error %S (%i)\n", s, error);
#endif
}

namespace tri {
	
	TcpSocket::TcpSocket() {
		handle = -1;
	}

	TcpSocket::~TcpSocket() {
		disconnect();
	}

	bool TcpSocket::connect(const Endpoint& ep) {
		if (handle != -1) {
			disconnect();
		}

		endpoint = ep;

		if (!endpoint.isValid()) {
			return false;
		}

		handle = socket(ep.isIpv4() ? AF_INET : AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		if (handle == -1) {
			printSocketError();
			return false;
		}

		int code = ::connect(handle, (sockaddr*)endpoint.getHandle(), sizeof(Endpoint));
		if (code != 0) {
			printSocketError();
			disconnect();
			return false;
		}

		return true;
	}

	bool TcpSocket::connect(const std::string& address, uint16_t port, bool resolve, bool prefereIpv4) {
		return connect(Endpoint(address, port, resolve, prefereIpv4));
	}

	bool TcpSocket::listen(uint16_t port, bool prefereIpv4) {
		struct sockaddr_in6 &addr6 = *(sockaddr_in6*)endpoint.getHandle();
		struct sockaddr_in& addr4 = *(sockaddr_in*)&addr6;
		memset(&addr6, 0, sizeof(addr6));
		
		if (prefereIpv4) {
			addr4.sin_family = AF_INET;
			addr4.sin_port = htons(port);
			addr4.sin_addr.S_un.S_addr = INADDR_ANY;
		}
		else {
			addr6.sin6_family = AF_INET6;
			addr6.sin6_port = htons(port);
			addr6.sin6_addr = in6addr_any;
		}

		if (handle != -1) {
			disconnect();
		}

		handle = socket(addr6.sin6_family, SOCK_STREAM, IPPROTO_TCP);
		if (handle == -1) {
			printSocketError();
			return false;
		}

		int flag = 1;
		setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));

		int code = bind(handle, (sockaddr*)&addr6, sizeof(addr6));
		if (code != 0) {
			printSocketError();
			disconnect();
			return false;
		}

		code = ::listen(handle, 10);
		if (code != 0) {
			printSocketError();
			disconnect();
			return false;
		}

		return true;
	}

	Ref<TcpSocket> TcpSocket::accept() {
		Endpoint ep;

		socklen_t size = sizeof(Endpoint);
		int result = ::accept(handle, (sockaddr*)ep.getHandle(), &size);
		if (result == -1) {
			printSocketError();
			disconnect();
			return nullptr;
		}

		Ref<TcpSocket> socket = Ref<TcpSocket>::make();
		socket->handle = result;
		socket->endpoint = ep;
		return socket;
	}

	bool TcpSocket::disconnect() {
#if TRI_WINDOWS
		int status = shutdown(handle, SD_BOTH);
		status = closesocket(handle);
		handle = -1;
#else
		int status = shutdown(handle, SHUT_RDWR);
		status = close(handle);
		handle = -1;
#endif
		return status == 0;
	}

	bool TcpSocket::isConnected() {
		return handle != -1;
	}

	const Endpoint& TcpSocket::getEndpoint() {
		return endpoint;
	}

	bool TcpSocket::write(void* data, int bytes) {
		int code = ::send(handle, (char*)data, bytes, 0);
		if (code < 0) {
			disconnect();
			return false;
		}
		return true;
	}

	bool TcpSocket::read(void* data, int& bytes) {
		int code = ::recv(handle, (char*)data, bytes, 0);
		if (code <= 0) {
			disconnect();
			return false;
		}
		bytes = code;
		return true;
	}

	int TcpSocket::getHandle() {
		return handle;
	}

}

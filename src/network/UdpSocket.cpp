//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "UdpSocket.h"
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

void printSocketError();

namespace tri {

	UdpSocket::UdpSocket() {}

	UdpSocket::~UdpSocket() {
		close();
	}

	bool UdpSocket::create(bool prefereIpv4) {
		handle = socket(prefereIpv4 ? AF_INET : AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		if (handle == -1) {
			printSocketError();
			return false;
		}
		return true;
	}

	bool UdpSocket::listen(uint16_t port, bool prefereIpv4) {
		struct sockaddr_in6 addr6;
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
			close();
		}

		handle = socket(addr6.sin6_family, SOCK_DGRAM, IPPROTO_UDP);
		if (handle == -1) {
			printSocketError();
			return false;
		}

		int code = ::bind(handle, (sockaddr*)&addr6, sizeof(addr6));
		if (code != 0) {
			printSocketError();
			close();
			return false;
		}

		return true;
	}

	void UdpSocket::close() {
#if TRI_WINDOWS
		int status = shutdown(handle, SD_BOTH);
		if (status == 0) {
			status = closesocket(handle);
			handle = -1;
		}
#else
		int status = shutdown(handle, SHUT_RDWR);
		if (status == 0) {
			status = close(handle);
			handle = -1;
		}
#endif
	}

	bool UdpSocket::write(void* data, int bytes, const Endpoint& endpoint) {
		int code = ::sendto(handle, (char*)data, bytes, 0, (sockaddr*)endpoint.getHandle(), sizeof(endpoint));
		if (code < 0) {
			return false;
		}
		return false;
	}

	bool UdpSocket::read(void* data, int& bytes, Endpoint& endpoint) {
		int size = sizeof(Endpoint);
		bytes = ::recvfrom(handle, (char*)data, bytes, 0, (sockaddr*)endpoint.getHandle(), &size);
		if (bytes <= 0) {
			return false;
		}
		return true;
	}

}

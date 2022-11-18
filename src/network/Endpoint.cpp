//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Endpoint.h"
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

namespace tri {

	Endpoint::Endpoint() {
		memset(data, 0, sizeof(data));
	}

	Endpoint::Endpoint(const std::string& address, uint16_t port, bool resolve, bool prefereIpv4) : Endpoint() {
		set(address, port, resolve, prefereIpv4);
	}

	Endpoint::~Endpoint() {}

	void Endpoint::setPort(uint16_t port) {
		struct sockaddr_in6& addr6 = *(sockaddr_in6*)data;
		addr6.sin6_port = htons(port);
	}

	void Endpoint::setAddress(const std::string& address, bool resolve, bool prefereIpv4) {
		struct sockaddr_in6 &addr6 = *(sockaddr_in6*)data;
		struct sockaddr_in &addr4 = *(sockaddr_in*)data;

		addr6.sin6_family = 0;
		uint16_t port = addr6.sin6_port;

		if (addr6.sin6_family == 0) {
			if (inet_pton(AF_INET, address.c_str(), &addr4.sin_addr) == 1) {
				addr4.sin_family = AF_INET;
			}
		}
		if (addr6.sin6_family == 0) {
			if (inet_pton(AF_INET6, address.c_str(), &addr6.sin6_addr) == 1) {
				addr6.sin6_family = AF_INET6;
			}
		}
		if (resolve && addr6.sin6_family == 0) {
			struct addrinfo* info;
			if (getaddrinfo(address.c_str(), nullptr, nullptr, &info) == 0) {
				for (addrinfo* i = info; i != nullptr; i = i->ai_next) {
					if (i->ai_family == AF_INET) {
						addr4 = *(sockaddr_in*)i->ai_addr;
						if (prefereIpv4) {
							break;
						}
					}
					else if (i->ai_family == AF_INET6) {
						addr6 = *(sockaddr_in6*)i->ai_addr;
						if (!prefereIpv4) {
							break;
						}
					}
				}
				freeaddrinfo(info);
			}
		}

		addr6.sin6_port = port;
	}

	void Endpoint::set(const std::string& address, uint16_t port, bool resolve, bool prefereIpv4) {
		setPort(port);
		setAddress(address, resolve, prefereIpv4);
	}

	uint16_t Endpoint::getPort() const {
		struct sockaddr_in6& addr6 = *(sockaddr_in6*)data;
		return htons(addr6.sin6_port);
	}

	std::string Endpoint::getAddress() const {
		struct sockaddr_in6& addr6 = *(sockaddr_in6*)data;
		struct sockaddr_in& addr4 = *(sockaddr_in*)data;
		char buf[INET6_ADDRSTRLEN];
		if (isIpv4()) {
			return inet_ntop(addr6.sin6_family, &addr4.sin_addr, buf, sizeof(buf));
		}
		else {
			return inet_ntop(addr6.sin6_family, &addr6.sin6_addr, buf, sizeof(buf));
		}
	}

	bool Endpoint::isIpv4() const {
		struct sockaddr_in6& addr6 = *(sockaddr_in6*)data;
		return addr6.sin6_family == AF_INET;
	}

	bool Endpoint::isIpv6() const {
		struct sockaddr_in6& addr6 = *(sockaddr_in6*)data;
		return addr6.sin6_family == AF_INET6;
	}

	bool Endpoint::isValid() const {
		struct sockaddr_in6& addr6 = *(sockaddr_in6*)data;
		return addr6.sin6_family == AF_INET || addr6.sin6_family == AF_INET6;
	}

	void* Endpoint::getHandle() {
		return (void*)data;
	}

	const void* Endpoint::getHandle() const {
		return (void*)data;
	}

}

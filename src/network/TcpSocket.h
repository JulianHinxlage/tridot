//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "Endpoint.h"
#include "core/util/Ref.h"

namespace tri {

	class TcpSocket {
	public:
		int bytesUp = 0;
		int bytesDown = 0;

		TcpSocket();
		~TcpSocket();

		bool connect(const Endpoint &endpoint);
		bool connect(const std::string& address, uint16_t port, bool resolve = true, bool prefereIpv4 = false);
		bool listen(uint16_t port, bool prefereIpv4 = false);
		Ref<TcpSocket> accept();
		bool disconnect();

		bool isConnected();
		const Endpoint& getEndpoint();

		bool write(const void* data, int bytes);
		bool read(void* data, int &bytes);

		int getHandle();
	private:
		Endpoint endpoint;
		bool connected;
		int handle;
	};

}

//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "Endpoint.h"

namespace tri {

	class UdpSocket {
	public:
		UdpSocket();
		~UdpSocket();

		bool create(bool prefereIpv4 = false);
		bool listen(uint16_t port, bool prefereIpv4 = false);
		void close();

		bool write(const void* data, int bytes, const Endpoint &endpoint);
		bool read(void* data, int &bytes, Endpoint& endpoint);

	private:
		int handle;
	};

}

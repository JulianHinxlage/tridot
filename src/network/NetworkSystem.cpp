//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "NetworkSystem.h"
#include "core/core.h"

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
	
	TRI_SYSTEM(NetworkSystem);

	void NetworkSystem::startup() {
#if TRI_WINDOWS
		WSADATA wsaData;
		int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (result != NO_ERROR) {
			env->console->error("WSA startup failed with code %i", result);
			return;
		}
#endif
	}

	void NetworkSystem::shutdown() {
#if TRI_WINDOWS
		WSACleanup();
#endif
	}

}

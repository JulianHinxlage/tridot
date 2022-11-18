//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/System.h"

namespace tri {

	class NetworkSystem : public System {
	public:
		void startup() override;
		void shutdown() override;
	};

}

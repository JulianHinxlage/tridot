//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/System.h"
#include "entity/ComponentStorage.h"

namespace tri {

	class PropertyReplication : public System {
	public:
		void init() override;
		void startup() override;
		void tick() override;

	private:
		std::vector<std::vector<std::shared_ptr<ComponentStorage>>> storages;
	};

}

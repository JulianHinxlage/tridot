//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "engine/Asset.h"
#include "core/util/Ref.h"

namespace tri {

	class Audio : public Asset {
	public:
		Audio();
		~Audio();
		virtual bool load(const std::string& file);
		uint32_t getId();
	
	private:
		uint32_t id;
	};

}

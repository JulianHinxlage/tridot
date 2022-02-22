//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/core.h"

namespace tri {

	class RenderSettings {
	public:
        bool shadowsEnabled = true;
        bool drawListSortingEnabled = true;
        bool frustumCullingEnabled = true;
        bool deferredShadingEnabled = true;
        bool transparencyPassEnabled = true;
	};

}
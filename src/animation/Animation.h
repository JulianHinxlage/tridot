//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "engine/MetaTypes.h"
#include "entity/Prefab.h"
#include "engine/Asset.h"

namespace tri {

	enum KeyBlendMode {
		NONE,
		STEP,
		LINEAR,
		SMOOTH,
	};

	class PropertyFrame {
	public:
		PropertyValueIdentifier value;
		float time;
		KeyBlendMode blend;
		bool relativeValue = false;
	};

	class PropertySequence {
	public:
		EntityId entityId = -1;
		std::vector<PropertyFrame> frames;
	};

	class Animation : public Asset {
	public:
		std::vector<PropertySequence> timeline;

		bool load(const std::string& file) override;
		bool save(const std::string& file) override;

		float calculateMaxTime();
		void apply(float time, EntityId id);
	};

}

//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "engine/MetaTypes.h"
#include "engine/Prefab.h"
#include "engine/Asset.h"

namespace tri {

	enum KeyFrameBlendMode {
		NONE,
		STEP,
		LINEAR,
		SMOOTH,
	};

	class KeyFrameProperty {
	public:
		PropertyValueIdentifier value;
	};

	class KeyFrame {
	public:
		float time = 0;
		KeyFrameBlendMode blend = NONE;
		std::vector<KeyFrameProperty> properties;
	};

	class Animation : public Asset {
	public:
		std::vector<KeyFrame> keyFrames;

		bool load(const std::string& file) override;
		bool save(const std::string& file) override;

		float calculateMaxTime();
		void apply(float time, EntityId id);
	};

}

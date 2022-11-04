//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "engine/EntityEvent.h"

namespace tri {

	class Hitbox {
	public:
		float hitPoints = -1;
		float maxHitPoints = -1;

		float hitDamage = 0;
		bool destroyOnImpact = false;
		int team = 0;
		float lifeTime = -1;

		EntityEvent onDestroy;
		EntityId lastHit = -1;
	};

}

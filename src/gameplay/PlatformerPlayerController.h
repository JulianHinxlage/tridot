//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"

namespace tri {

	class PlatformerPlayerController {
	public:
		float speed = 10;
		float drag = 10;
		float maxFallSpeed = 20;
		float jumpHeight = 5;
		int jumpCount = 1;
		float jumpGracePeriode = 0.15;

		bool active = true;
		float lastGroundContactTime = 0;
		float lastJumpInputTime = -1;
		bool onGround = false;
		int hasJumpedCounter = 0;
	};

}

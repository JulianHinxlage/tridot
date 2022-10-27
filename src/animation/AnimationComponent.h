//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "Animation.h"

namespace tri {

	class AnimationComponent {
	public:
		Ref<Animation> animation;
		float speed = 1;
		bool playing = false;
		bool reversed = false;
		bool looping = false;
		bool pingPong = false;

		float time = 0;
		float maxTime = 0;

		void play();
		void stop();
		void unpause();
		void playReverse();
	};

}

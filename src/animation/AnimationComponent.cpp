//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "AnimationComponent.h"
#include "core/core.h"
#include "engine/Time.h"

namespace tri {

	TRI_COMPONENT(AnimationComponent);
	TRI_PROPERTIES6(AnimationComponent, animation, speed, playing, reversed, looping, pingPong);
	TRI_FUNCTION(AnimationComponent, play);
	TRI_FUNCTION(AnimationComponent, stop);
	TRI_FUNCTION(AnimationComponent, unpause);
	TRI_FUNCTION(AnimationComponent, playReverse);

	void AnimationComponent::play() {
		playing = true;

		if (animation) {
			maxTime = animation->calculateMaxTime();
		}

		reversed = false;
		//if (reversed) {
		//	time = maxTime;
		//}
		//else {
		//	time = 0;
		//}
	}

	void AnimationComponent::stop() {
		playing = false;
	}

	void AnimationComponent::unpause() {
		playing = true;
	}

	void AnimationComponent::playReverse() {
		playing = true;

		if (animation) {
			maxTime = animation->calculateMaxTime();
		}

		reversed = true;
		//if (reversed) {
		//	time = maxTime;
		//}
		//else {
		//	time = 0;
		//}
	}

}

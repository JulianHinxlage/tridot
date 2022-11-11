//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "Audio.h"
#include "engine/EntityEvent.h"

namespace tri {

	class AudioListener {
	public:
		float volume = 1.0f;
	};

	class AudioSource {
	public:
		Ref<Audio> audio;
		float volume = 1.0f;
		float pitch = 1.0f;
		bool positional = true;
		bool looping = false;
		uint32_t id = 0;

		void play();
		void stop();
	};

}

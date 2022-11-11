//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "AudioSource.h"
#include "core/core.h"
#include "AudioSystem.h"

namespace tri {

	TRI_COMPONENT_CATEGORY(AudioListener, "Audio");
	TRI_PROPERTIES1(AudioListener, volume);

	TRI_COMPONENT_CATEGORY(AudioSource, "Audio");
	TRI_PROPERTIES5(AudioSource, audio, volume, pitch, positional, looping);
	TRI_FUNCTION(AudioSource, play);
	TRI_FUNCTION(AudioSource, stop);

	void AudioSource::play() {
		if (looping && id != 0) {
			env->audioSystem->stop(id);
			id = 0;
		}
		id = env->audioSystem->play(audio);
	}

	void AudioSource::stop() {
		env->audioSystem->stop(id);
		id = 0;
	}

}

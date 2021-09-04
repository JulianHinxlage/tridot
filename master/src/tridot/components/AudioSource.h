//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "tridot/engine/Audio.h"
#include "tridot/util/Ref.h"

namespace tridot {

    class AudioSource {
    public:
        uint32_t id;
        uint32_t playingAudioId;
        Ref<Audio> audio;

        AudioSource();
    };

    class AudioListener{
    public:

    };

}


//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "Audio.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace tridot {

    class AudioManager {
    public:
        AudioManager();
        ~AudioManager();
        void init();
        void update();
        void play(const std::shared_ptr<Audio> &audio);
        void setListenerPosition(const glm::vec3 &position);

    private:
        std::vector<uint32_t> sources;
        std::vector<std::shared_ptr<Audio>> loadingQueue;
    };

}


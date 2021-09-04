//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "Audio.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <unordered_set>

namespace tridot {

    class AudioManager {
    public:
        AudioManager();
        ~AudioManager();
        void init();
        void update();
        void shutdown();
        uint32_t play(const std::shared_ptr<Audio> &audio, bool loop = false);
        void setListenerPosition(const glm::vec3 &position);
        glm::vec3 getListenerPosition();
        void setListenerRotation(const glm::vec3 &rotation);
        void setListenerVelocity(const glm::vec3 &velocity);
        void setListenerVolume(float volume);
        void setListenerPitch(float pitch);
        void setPosition(uint32_t id, const glm::vec3 &position);
        void setVolume(uint32_t id, float volume);
        void setVelocity(uint32_t id, const glm::vec3 &velocity);
        void setPitch(uint32_t id, float pitch);
        void stop(uint32_t id);
        void pause(uint32_t id);
        void resume(uint32_t id);

    private:
        std::unordered_set<uint32_t> sources;
        std::vector<std::shared_ptr<Audio>> loadingQueue;
        glm::vec3 listenerPosition;
    };

}


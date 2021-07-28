//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "AudioManager.h"
#include "tridot/core/Environment.h"
#include <glm/ext/matrix_transform.hpp>

#if TRI_ENABLE_AUDIO

#include <AL/al.h>
#include <AL/alut.h>
#include <AL/alc.h>

namespace tridot {

    AudioManager::AudioManager() {

    }

    AudioManager::~AudioManager() {
        shutdown();
    }

    void AudioManager::init() {
        ALCdevice *device = alcOpenDevice(nullptr);

        if(!device){
            env->console->error("could not open audio device");
            return;
        }

        ALCcontext *context = alcCreateContext(device, nullptr);
        if(!context){
            env->console->error("could not create audio context");
            return;
        }
        alcMakeContextCurrent((ALCcontext*)context);
        if(alutInitWithoutContext(nullptr, nullptr) == AL_FALSE){
            env->console->error("could not initialize ALUT: ", alutGetErrorString(alutGetError()));
            return;
        }

        env->console->info("OpenAl version: ", alGetString(AL_VERSION));
        env->console->info("OpenAl vendor: ", alGetString(AL_VENDOR));
        setListenerPosition({0, 0, 0});
    }

    void AudioManager::update() {
        for(auto i = sources.begin(); i != sources.end();){
            uint32_t id = *i;
            int state = 0;
            alGetSourcei(id, AL_SOURCE_STATE, &state);
            if(state == AL_STOPPED) {
                alDeleteSources(1, &id);
                sources.erase(i++);
            }else{
                ++i;
            }
        }
        for(int i = 0; i < loadingQueue.size(); i++){
            auto &audio = loadingQueue[i];
            if(audio->getId() != 0){
                play(audio);
                loadingQueue.erase(loadingQueue.begin() + i);
                i--;
            }
        }
    }

    uint32_t AudioManager::play(const std::shared_ptr<Audio> &audio, bool loop) {
        if(audio->getId() == 0){
            loadingQueue.push_back(audio);
            return 0;
        }else {
            uint32_t id = 0;
            alGenSources(1, &id);
            alSourcei(id, AL_BUFFER, audio->getId());
            alSourcei(id, AL_LOOPING, loop);
            alSource3f(id, AL_POSITION, 0, 0, 0);
            alSourcePlay(id);
            sources.insert(id);
            return id;
        }
        return 0;
    }

    void AudioManager::setListenerPosition(const glm::vec3 &position) {
        alListener3f(AL_POSITION, position.x, position.y, position.z);
        listenerPosition = position;
    }

    glm::vec3 AudioManager::getListenerPosition() {
        return listenerPosition;
    }

    void AudioManager::setListenerRotation(const glm::vec3 &rotation) {
        glm::mat4 transform(1);
        transform = glm::rotate(transform, rotation.z, {0, 0, 1});
        transform = glm::rotate(transform, rotation.y, {0, 1, 0});
        transform = glm::rotate(transform, rotation.x, {1, 0, 0});
        glm::vec3 forward = transform * glm::vec4(0, 1, 0, 0);
        glm::vec3 up = transform * glm::vec4(0, 0, 1, 0);
        float orientation[] = {forward.x, forward.y, forward.z, up.x, up.y, up.z};
        alListenerfv(AL_ORIENTATION, orientation);
    }

    void AudioManager::setListenerVelocity(const glm::vec3 &velocity) {
        alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z);
    }

    void AudioManager::setListenerVolume(float volume) {
        alListenerf(AL_GAIN, volume);
    }

    void AudioManager::setListenerPitch(float pitch) {
        alListenerf(AL_PITCH, pitch);
    }

    void AudioManager::setPosition(uint32_t id, const glm::vec3 &position) {
        alSource3f(id, AL_POSITION, position.x, position.y, position.z);
    }

    void AudioManager::setVolume(uint32_t id, float volume) {
        alSourcef(id, AL_GAIN, volume);
    }

    void AudioManager::setVelocity(uint32_t id, const glm::vec3 &velocity) {
        alSource3f(id, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
    }

    void AudioManager::setPitch(uint32_t id, float pitch) {
        alSourcef(id, AL_PITCH, pitch);
    }

    void AudioManager::stop(uint32_t id) {
        alSourceStop(id);
        alDeleteSources(1, &id);
        sources.erase(id);
    }

    void AudioManager::pause(uint32_t id) {
        alSourcePause(id);
    }

    void AudioManager::resume(uint32_t id) {
        alSourcePlay(id);
    }

    void AudioManager::shutdown() {
        loadingQueue.clear();
        for(auto &id : sources){
            alDeleteSources(1, &id);
        }
        sources.clear();
        ALCcontext *context = alcGetCurrentContext();
        ALCdevice *device = alcGetContextsDevice(context);
        alcMakeContextCurrent(nullptr);
        if(context){
            alcDestroyContext((ALCcontext *)context);
        }
        if(device){
            alcCloseDevice((ALCdevice*)device);
        }
    }

}

#else


namespace tridot {

    AudioManager::AudioManager() {}

    AudioManager::~AudioManager() {
        shutdown();
    }

    void AudioManager::init() {}
    void AudioManager::update() {}

    uint32_t AudioManager::play(const std::shared_ptr<Audio>& audio, bool loop) {
        return 0;
    }

    void AudioManager::setListenerPosition(const glm::vec3& position) {
        listenerPosition = position;
    }

    glm::vec3 AudioManager::getListenerPosition() {
        return listenerPosition;
    }

    void AudioManager::setListenerRotation(const glm::vec3& rotation) {}
    void AudioManager::setListenerVelocity(const glm::vec3& velocity) {}
    void AudioManager::setListenerVolume(float volume) {}
    void AudioManager::setListenerPitch(float pitch) {}
    void AudioManager::setPosition(uint32_t id, const glm::vec3& position) {}
    void AudioManager::setVolume(uint32_t id, float volume) {}
    void AudioManager::setVelocity(uint32_t id, const glm::vec3& velocity) {}
    void AudioManager::setPitch(uint32_t id, float pitch) {}
    void AudioManager::stop(uint32_t id) {}
    void AudioManager::pause(uint32_t id) {}
    void AudioManager::resume(uint32_t id) {}
    void AudioManager::shutdown() {}

}

#endif
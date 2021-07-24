//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "AudioManager.h"
#include "tridot/core/Environment.h"
#include <AL/al.h>
#include <AL/alut.h>
#include <AL/alc.h>

namespace tridot {

    AudioManager::AudioManager() {

    }

    AudioManager::~AudioManager() {
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
        for(int i = 0; i < sources.size(); i++){
            uint32_t id = sources[i];
            int state = 0;
            alGetSourcei(id, AL_SOURCE_STATE, &state);
            if(state == AL_STOPPED){
                alDeleteSources(1, &id);
                sources.erase(sources.begin() + i);
                i--;
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

    void AudioManager::play(const std::shared_ptr<Audio> &audio) {
        if(audio->getId() == 0){
            loadingQueue.push_back(audio);
        }else {
            uint32_t id = 0;
            alGenSources(1, &id);
            alSourcei(id, AL_BUFFER, audio->getId());
            alSourcei(id, AL_LOOPING, false);
            alSource3f(id, AL_POSITION, 0, 0, 0);
            alSourcePlay(id);
            sources.push_back(id);
        }
    }

    void AudioManager::setListenerPosition(const glm::vec3 &position) {
        alListener3f(AL_POSITION, position.x, position.y, position.z);
        alListener3f(AL_VELOCITY, 0, 0, 0);
        alListenerf(AL_GAIN, 1);
    }

}

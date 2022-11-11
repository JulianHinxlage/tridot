//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "AudioSystem.h"
#include "Audio.h"
#include "AudioSource.h"
#include "core/core.h"
#include "editor/Editor.h"
#include "engine/RuntimeMode.h"
#include "engine/Transform.h"
#include "AL/al.h"
#include "AL/alc.h"

namespace tri {

	TRI_SYSTEM_INSTANCE(AudioSystem, env->audioSystem);

	void AudioSystem::init() {
        env->systemManager->addSystem<RuntimeMode>();
		if (env->editor) {
			env->editor->fileAssosiations[".wav"] = Reflection::getClassId<Audio>();
		}
        env->runtimeMode->setActiveSystem(RuntimeMode::EDIT, "AudioSystem", true);
        env->runtimeMode->setActiveSystem(RuntimeMode::PAUSED, "AudioSystem", true);
	}

	void AudioSystem::startup() {
        ALCdevice* device = alcOpenDevice(nullptr);

        if (!device) {
            env->console->error("could not open audio device");
            return;
        }

        ALCcontext* context = alcCreateContext(device, nullptr);
        if (!context) {
            env->console->error("could not create audio context");
            return;
        }
        alcMakeContextCurrent((ALCcontext*)context);

        env->console->info("OpenAl version: %s", alGetString(AL_VERSION));
        env->console->info("OpenAl vendor: %s", alGetString(AL_VENDOR));

        alListener3f(AL_POSITION, 0, 0, 0);
        alListener3f(AL_VELOCITY, 0, 0, 0);
        alListenerf(AL_GAIN, 1);
	}

	void AudioSystem::tick() {
        env->world->each<AudioListener, Transform>([&](AudioListener &listener, Transform &transform) {
            alListener3f(AL_POSITION, transform.position.x, transform.position.y, transform.position.z);
            alListener3f(AL_VELOCITY, 0, 0, 0);
            alListenerf(AL_GAIN, listener.volume);
        });
        env->world->each<AudioSource, Transform>([&](AudioSource& source, Transform& transform) {
            if (source.id != 0) {
                if (source.positional) {
                    alSourcei(source.id, AL_SOURCE_RELATIVE, AL_FALSE);
                    alSource3f(source.id, AL_POSITION, transform.position.x, transform.position.y, transform.position.z);
                }
                else {
                    alSourcei(source.id, AL_SOURCE_RELATIVE, AL_TRUE);
                }
                alSourcef(source.id, AL_GAIN, source.volume);
                alSourcef(source.id, AL_PITCH, source.pitch);
                alSourcei(source.id, AL_LOOPING, source.looping);
            }
        });

        for (int i = 0; i < sources.size(); i++) {
            uint32_t id = sources[i];
            int state = 0;
            alGetSourcei(id, AL_SOURCE_STATE, &state);
            if (state == AL_STOPPED) {
                alDeleteSources(1, &id);
                sources.erase(sources.begin() + i);
                i--;
            }
        }

        for (int i = 0; i < loadingQueue.size(); i++) {
            auto& audio = loadingQueue[i];
            if (audio->getId() != 0) {
                play(audio);
                loadingQueue.erase(loadingQueue.begin() + i);
                i--;
            }
        }
	}

	void AudioSystem::shutdown() {
        for (int i = 0; i < sources.size(); i++) {
            uint32_t id = sources[i];
            alDeleteSources(1, &id);
        }
        loadingQueue.clear();
        sources.clear();

        ALCcontext* context = alcGetCurrentContext();
        ALCdevice* device = alcGetContextsDevice(context);
        alcMakeContextCurrent(nullptr);
        if (context) {
            alcDestroyContext((ALCcontext*)context);
        }
        if (device) {
            alcCloseDevice((ALCdevice*)device);
        }
	}

    uint32_t AudioSystem::play(const Ref<Audio>& audio, const glm::vec3& pos) {
        if (!audio) {
            return 0;
        }
        if (audio->getId() == 0) {
            loadingQueue.push_back(audio);
            return 0;
        }
        else {
            uint32_t id = 0;
            alGenSources(1, &id);
            alSourcei(id, AL_BUFFER, audio->getId());
            alSourcei(id, AL_LOOPING, false);
            alSource3f(id, AL_POSITION, pos.x, pos.y, pos.z);
            alSourcePlay(id);
            sources.push_back(id);
            return id;
        }
    }

    void AudioSystem::stop(uint32_t id) {
        for (int i = 0; i < sources.size(); i++) {
            if (sources[i] == id) {
                sources.erase(sources.begin() + i);
                i--;
            }
        }
        alDeleteSources(1, &id);
    }

}

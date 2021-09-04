//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Audio.h"
#include "tridot/core/Environment.h"

#if TRI_ENABLE_AUDIO

#include <AL/al.h>
#include <AL/alut.h>

namespace tridot {

    Audio::Audio() {
        id = 0;
    }

    Audio::~Audio() {
        clear();
    }

    bool Audio::preLoad(const std::string& filename) {
        if(!alcGetCurrentContext()){
            env->console->warning("audio manager not initialized");
            return false;
        }

        ALenum format = 0;
        int size = 0;
        float frequency = 0;

        char *data = (char*)alutLoadMemoryFromFile(filename.c_str(), &format, &size, &frequency);
        ALenum error = alutGetError();
        if(error){
            env->console->warning("could not load audio file ", filename, ": ", alutGetErrorString(error));
            return false;
        }

        if(id == 0){
            alGenBuffers(1, &id);
            env->console->trace("created audio buffer ", id);
        }

        if(format == AL_FORMAT_STEREO8){
            //convert stereo to mono
            char *monoData = new char[size / 2];
            for (int i = 0; i < size; i += 2) {
                char left = *(char *) &data[i];
                char right = *(char *) &data[i + 1];
                monoData[i / 2] = ((int) left + right) / 2;
            }
            alBufferData(id, AL_FORMAT_MONO8, monoData, size / 2, frequency);
            delete[] monoData;
        }else if(format == AL_FORMAT_STEREO16){
            //convert stereo to mono
            short *monoData = new short[size / 4];
            for (int i = 0; i < size; i += 4) {
                short left = *(short *) &data[i];
                short right = *(short *) &data[i + 2];
                monoData[i / 4] = ((int) left + right) / 2;
            }
            alBufferData(id, AL_FORMAT_MONO16, monoData, size / 2, frequency);
            delete[] monoData;
        }else if(format == AL_FORMAT_MONO8){
            alBufferData(id, format, data, size, frequency);
        }else if(format == AL_FORMAT_MONO16){
            alBufferData(id, format, data, size, frequency);
        }
        free(data);
        env->console->debug("loaded audio ", filename);
        return true;
    }

    bool Audio::postLoad() {
        return id != 0;
    }

    uint32_t Audio::getId() {
        return id;
    }

    void Audio::clear() {
        if (id != 0) {
            alDeleteBuffers(1, &id);
            id = 0;
        }
    }

}

#else

namespace tridot {

    Audio::Audio() {
        id = 0;
    }

    Audio::~Audio() {
        clear();
    }

    bool Audio::preLoad(const std::string& filename) {
        return false;
    }

    bool Audio::postLoad() {
        return id != 0;
    }

    uint32_t Audio::getId() {
        return id;
    }

    void Audio::clear() {}

}

#endif

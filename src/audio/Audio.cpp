//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Audio.h"
#include "core/core.h"
#include "AL/al.h"
#include "AL/alc.h"
#include <iostream>

std::int32_t convert_to_int(char* buffer, std::size_t len)
{
    std::int32_t a = 0;
    if (std::endian::native == std::endian::little)
        std::memcpy(&a, buffer, len);
    else
        for (std::size_t i = 0; i < len; ++i)
            reinterpret_cast<char*>(&a)[3 - i] = buffer[i];
    return a;
}

bool load_wav_file_header(std::ifstream& file,
    std::uint8_t& channels,
    std::int32_t& sampleRate,
    std::uint8_t& bitsPerSample,
    ALsizei& size)
{
    char buffer[4];
    if (!file.is_open())
        return false;

    // the RIFF
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read RIFF" << std::endl;
        return false;
    }
    if (std::strncmp(buffer, "RIFF", 4) != 0)
    {
        std::cerr << "ERROR: file is not a valid WAVE file (header doesn't begin with RIFF)" << std::endl;
        return false;
    }

    // the size of the file
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read size of file" << std::endl;
        return false;
    }

    // the WAVE
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read WAVE" << std::endl;
        return false;
    }
    if (std::strncmp(buffer, "WAVE", 4) != 0)
    {
        std::cerr << "ERROR: file is not a valid WAVE file (header doesn't contain WAVE)" << std::endl;
        return false;
    }

    // "fmt/0"
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read fmt/0" << std::endl;
        return false;
    }

    // this is always 16, the size of the fmt data chunk
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read the 16" << std::endl;
        return false;
    }

    // PCM should be 1?
    if (!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read PCM" << std::endl;
        return false;
    }

    // the number of channels
    if (!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read number of channels" << std::endl;
        return false;
    }
    channels = convert_to_int(buffer, 2);

    // sample rate
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read sample rate" << std::endl;
        return false;
    }
    sampleRate = convert_to_int(buffer, 4);

    // (sampleRate * bitsPerSample * channels) / 8
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read (sampleRate * bitsPerSample * channels) / 8" << std::endl;
        return false;
    }

    // ?? dafaq
    if (!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read dafaq" << std::endl;
        return false;
    }

    // bitsPerSample
    if (!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read bits per sample" << std::endl;
        return false;
    }
    bitsPerSample = convert_to_int(buffer, 2);

    // data chunk header "data"
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read data chunk header" << std::endl;
        return false;
    }
    if (std::strncmp(buffer, "data", 4) != 0)
    {
        std::cerr << "ERROR: file is not a valid WAVE file (doesn't have 'data' tag)" << std::endl;
        return false;
    }

    // size of data
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read data size" << std::endl;
        return false;
    }
    size = convert_to_int(buffer, 4);

    /* cannot be at the end of file */
    if (file.eof())
    {
        std::cerr << "ERROR: reached EOF on the file" << std::endl;
        return false;
    }
    if (file.fail())
    {
        std::cerr << "ERROR: fail state set on the file" << std::endl;
        return false;
    }

    return true;
}

char* load_wav(const std::string& filename,
    std::uint8_t& channels,
    std::int32_t& sampleRate,
    std::uint8_t& bitsPerSample,
    ALsizei& size)
{
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open())
    {
        std::cerr << "ERROR: Could not open \"" << filename << "\"" << std::endl;
        return nullptr;
    }
    if (!load_wav_file_header(in, channels, sampleRate, bitsPerSample, size))
    {
        std::cerr << "ERROR: Could not load wav header of \"" << filename << "\"" << std::endl;
        return nullptr;
    }

    char* data = new char[size];

    in.read(data, size);

    return data;
}

namespace tri {

	TRI_ASSET(Audio);

	Audio::Audio() {
		id = 0;
	}

	Audio::~Audio() {
		if (id != 0) {
			alDeleteBuffers(1, &id);
			id = 0;
		}
	}

	bool Audio::load(const std::string& file) {
        if (!alcGetCurrentContext()) {
            env->console->warning("audio system not initialized");
            return false;
        }

        uint8_t channels = 0;
        int32_t sampleRate = 0;
        uint8_t bitsPerSample = 0;
        ALsizei size = 0;
        char *data = load_wav(file, channels, sampleRate, bitsPerSample, size);
        ALenum format = 0;
        if (channels == 1) {
            if (bitsPerSample == 8) {
                format = AL_FORMAT_MONO8;
            }else if (bitsPerSample == 16) {
                format = AL_FORMAT_MONO16;
            }
        }else if (channels == 2) {
            if (bitsPerSample == 8) {
                format = AL_FORMAT_STEREO8;
            }
            else if (bitsPerSample == 16) {
                format = AL_FORMAT_STEREO16;
            }
        }

        if (data == nullptr) {
            env->console->warning("could not load audio %s", file.c_str());
            return false;
        }

        if (id == 0) {
            alGenBuffers(1, &id);
        }

        if (format == AL_FORMAT_STEREO8) {
            //convert stereo to mono
            char* monoData = new char[size / 2];
            for (int i = 0; i < size; i += 2) {
                char left = *(char*)&data[i];
                char right = *(char*)&data[i + 1];
                monoData[i / 2] = ((int)left + right) / 2;
            }
            alBufferData(id, AL_FORMAT_MONO8, monoData, size / 2, sampleRate);
            delete[] monoData;
        }
        else if (format == AL_FORMAT_STEREO16) {
            //convert stereo to mono
            short* monoData = new short[size / 4];
            for (int i = 0; i < size; i += 4) {
                short left = *(short*)&data[i];
                short right = *(short*)&data[i + 2];
                monoData[i / 4] = ((int)left + right) / 2;
            }
            alBufferData(id, AL_FORMAT_MONO16, monoData, size / 2, sampleRate);
            delete[] monoData;
        }
        else if (format == AL_FORMAT_MONO8) {
            alBufferData(id, format, data, size, sampleRate);
        }
        else if (format == AL_FORMAT_MONO16) {
            alBufferData(id, format, data, size, sampleRate);
        }
        free(data);
        return true;
	}

    uint32_t Audio::getId() {
        return id;
    }

}

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_TEXTURE_H
#define TRIDOT_TEXTURE_H

#include "Image.h"
#include <cstdint>

namespace tridot {

    class Texture {
    public:
        Texture();
        ~Texture();

        void bind(int32_t slot);
        void unbind();
        uint32_t getId();

        void create(uint32_t width, uint32_t height, uint32_t channels, bool depth = false, bool stencil = false);
        bool load(const Image &image);
        bool load(const std::string &file);
        bool preLoad(const std::string &file);
        bool postLoad();

        void setMagMin(bool magNearest, bool minNearest);
        void setWrap(bool sRepeat, bool tRepeat);

        uint32_t getWidth();
        uint32_t getHeight();
        uint32_t getChannels();

    private:
        uint32_t id;
        int32_t slot;
        Image image;
        uint32_t width;
        uint32_t height;
        uint32_t channels;

        bool magNearest;
        bool minNearest;
        bool sRepeat;
        bool tRepeat;
    };

}

#endif //TRIDOT_TEXTURE_H
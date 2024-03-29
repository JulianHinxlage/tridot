//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "Image.h"
#include "enum.h"
#include "engine/Asset.h"

namespace tri {

    class Texture : public Asset {
    public:
        Texture();
        ~Texture();

        void bindAsImage(int32_t slot);
        void bind(int32_t slot);
        void unbind();
        uint32_t getId();

        void create(uint32_t width, uint32_t height, TextureFormat formal = RGBA8, bool enableMipMapping = true);
        bool load(const Image &image);
        bool load(const std::string &file);
        bool loadActivate();

        void setMagMin(bool magNearest, bool minNearest);
        void setWrap(bool sRepeat, bool tRepeat);
        void setBorderColor(Color color);

        uint32_t getWidth();
        uint32_t getHeight();
        uint32_t getChannels();
        TextureFormat getFormat();
        TextureType getType();
        Color getPixel(int x, int y);
        void clear(Color color);
        void setCubeMap(bool enableMipMapping = false);

    private:
        uint32_t id;
        int32_t slot;
        Image image;
        uint32_t width;
        uint32_t height;
        uint32_t channels;
        uint32_t mipCount;
        TextureFormat format;
        TextureType type;

        bool magNearest;
        bool minNearest;
        bool sRepeat;
        bool tRepeat;
    };

}


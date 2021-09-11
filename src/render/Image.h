//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "Color.h"
#include <vector>
#include <string>

namespace tri {

    class Image {
    public:
        Image();
        ~Image();
        Image(const Image &image);

        void init(int width, int height, int channels, int bitsPerChannel, Color *data = nullptr, int size = 0);
        void set(int x, int y, Color color);
        void set(int width, int height,int x, int y, Color *data, int size);
        Color get(int x, int y) const;
        void get(int width, int height,int x, int y, Color *data, int size) const;
        bool load(const std::string &file);
        uint32_t getWidth() const;
        uint32_t getHeight() const;
        uint32_t getChannels() const;
        uint32_t getBitsPerChannel() const;
        const uint8_t *getData() const;
        void clear();

    private:
        std::vector<uint8_t> data;
        uint32_t width;
        uint32_t height;
        uint32_t channels;
        uint32_t bitsPerChannel;
    };

}


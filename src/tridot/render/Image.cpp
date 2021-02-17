//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Image.h"
#include "tridot/core/Log.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace tridot {

    Image::Image() {
        width = 0;
        height = 0;
        channels = 0;
        bitsPerChannel = 0;
    }

    Image::~Image() {}
    Image::Image(const Image &image) {}

    void Image::init(int width, int height, int channels, int bitsPerChannel, Color *data, int size) {
        this->width = width;
        this->height = height;
        this->channels = channels;
        this->bitsPerChannel = bitsPerChannel;
        if(bitsPerChannel != 8){
            Log::error("only 8 bits per channel are supported");
        }
        this->data.resize(width * height * channels, 255);
        if(data != nullptr){
            for(int i = 0; i < std::min(size, width * height); i++){
                for(int j = 0; j < channels; j++){
                    this->data[i * channels + j] = *((uint8_t*)&data[i].value + j);
                }
            }
        }
    }

    void Image::set(int x, int y, Color color) {
        if(x >= 0 && y >= 0 && x < width && y < height){
            int i = y * width + x;
            for(int j = 0; j < channels; j++){
                this->data[i * channels + j] = *((uint8_t*)&color.value + j);
            }
        }
    }

    void Image::set(int width, int height, int x, int y, Color *data, int size) {
        for(int i = 0; i < width; i++){
            for(int j = 0; j < height; j++){
                if(size > j * width + i){
                    set(x + i, y + i, data[j * width + i]);
                }
            }
        }
    }

    Color Image::get(int x, int y) const {
        if(x >= 0 && y >= 0 && x < width && y < height){
            int i = y * width + x;
            Color color;
            for(int j = 0; j < channels; j++){
                *((uint8_t*)&color.value + j) = this->data[i * channels + j];
            }
            return color;
        }else{
            return Color::white;
        }
    }

    void Image::get(int width, int height, int x, int y, Color *data, int size) const {
        for(int i = 0; i < width; i++){
            for(int j = 0; j < height; j++){
                if(size > j * width + i){
                    data[j * width + i] = get(x + i, y + i);
                }
            }
        }
    }

    bool Image::load(const std::string &file) {
        int x, y, c;
        stbi_uc *uc = stbi_load(file.c_str(), &x, &y, &c, 0);

        if(uc == nullptr){
            Log::warning("could not load image ", file);
            return false;
        }

        width = x;
        height = y;
        channels = c;
        bitsPerChannel = 8;

        data.resize(width * height * channels);
        for(int i = 0; i < data.size(); i++){
            data[i] = uc[i];
        }
        stbi_image_free(uc);
        return true;
    }

    uint32_t Image::getWidth() const {
        return width;
    }

    uint32_t Image::getHeight() const {
        return height;
    }

    uint32_t Image::getChannels() const {
        return channels;
    }

    uint32_t Image::getBitsPerChannel() const {
        return bitsPerChannel;
    }

    const uint8_t *Image::getData() const {
        return data.data();
    }

    void Image::clear() {
        data.clear();
        width = 0;
        height = 0;
        channels = 0;
        bitsPerChannel = 0;
    }

}
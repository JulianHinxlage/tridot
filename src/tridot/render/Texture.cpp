//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Texture.h"
#include "tridot/core/Log.h"
#include <GL/glew.h>
#include <algorithm>

namespace tridot {

    Texture::Texture() {
        id = 0;
        slot = -1;

        width = 0;
        height = 0;
        channels = 0;
        mipCount = 0;

        magNearest = true;
        minNearest = false;
        sRepeat = true;
        tRepeat = true;
    }

    Texture::~Texture() {
        if(id != 0){
            glDeleteTextures(1, &id);
            Log::trace("deleted texture ", id);
            id = 0;
        }
    }

    void bindTexture(uint32_t id) {
        static int32_t currentId;
        if (currentId != id) {
            glBindTexture(GL_TEXTURE_2D, id);
            currentId = id;
        }
    }

    void bindTexture(uint32_t id, int32_t slot){
        static int32_t currentIds[128];
        if(slot >= 0 && currentIds[slot] != id){
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, id);
            currentIds[slot] = id;
        }
    }

    void Texture::bind(int32_t slot) {
        bindTexture(id, slot);
        this->slot = slot;
    }

    void Texture::unbind() {
        bindTexture(0, slot);
        slot = -1;
    }

    uint32_t Texture::getId() {
        return id;
    }

    void Texture::setMagMin(bool magNearest, bool minNearest) {
        bindTexture(id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magNearest ? GL_NEAREST : GL_LINEAR);
        if(mipCount > 1){
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minNearest ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
        }else{
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minNearest ? GL_NEAREST : GL_LINEAR);
        }
        this->magNearest = magNearest;
        this->minNearest = minNearest;
    }

    void Texture::setWrap(bool sRepeat, bool tRepeat) {
        bindTexture(id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sRepeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tRepeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
        this->sRepeat = sRepeat;
        this->tRepeat = tRepeat;
    }

    void Texture::create(uint32_t width, uint32_t height, TextureFormat format, bool enableMipMapping) {
        if(id != 0){
            glDeleteTextures(1, &id);
            glGenTextures(1, &id);
        }
        if(id == 0){
            glGenTextures(1, &id);
            Log::trace("created texture ", id);
        }

        this->width = width;
        this->height = height;
        this->channels = internalEnumSize(format) / 8;
        this->format = format;

        mipCount = std::max(1, (int)std::log2(std::min(width, height)) - 1);
        if(!enableMipMapping){
            mipCount = 1;
        }

        bind(0);
        setMagMin(magNearest, minNearest);
        setWrap(sRepeat, tRepeat);
        glTexStorage2D(GL_TEXTURE_2D, mipCount, internalEnum(format), width, height);
    }

    bool Texture::load(const Image &image) {
        GLenum dataFormat = GL_RGBA;
        TextureFormat format = RGBA8;
        if(image.getChannels() == 1){
            dataFormat = GL_RED;
            format = RED8;
        }else if(image.getChannels() == 2){
            dataFormat = GL_RG;
            format = RG8;
        }else if(image.getChannels() == 3){
            dataFormat = GL_RGB;
            format = RGB8;
        }else if(image.getChannels() == 4){
            dataFormat = GL_RGBA;
            format = RGBA8;
        }

        if(image.getBitsPerChannel() != 8){
            Log::error("only 8 bits per channel are supported");
        }

        create(image.getWidth(), image.getHeight(), format);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image.getWidth(), image.getHeight(), dataFormat, GL_UNSIGNED_BYTE, image.getData());
        if(mipCount > 1){
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        return true;
    }

    bool Texture::load(const std::string &file) {
        if(!preLoad(file)){
            return false;
        }
        return postLoad();
    }

    bool Texture::preLoad(const std::string &file) {
        return image.load(file);
    }

    bool Texture::postLoad() {
        if(load(image)){
            image.clear();
            return true;
        }else{
            image.clear();
            return false;
        }
    }

    uint32_t Texture::getWidth() {
        return width;
    }

    uint32_t Texture::getHeight() {
        return height;
    }

    uint32_t Texture::getChannels() {
        return channels;
    }

    TextureFormat Texture::getFormat() {
        return format;
    }

    Color Texture::getPixel(int x, int y) {
#ifndef TRI_USE_GL_4_5
        if ((int)width < 0 || (int)height < 0) {
            return Color(0, 0, 0, 0);
        }
        std::vector<Color> colors;
        int index = x + y * width;
        if (index >= width * height) {
            return Color(0, 0, 0, 0);
        }
        colors.resize(width * height);
        bind(0);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, colors.data());
        return colors[index];
#else
        Color color;
        glGetTextureSubImage(id, 0, x, y, 0, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, sizeof(color), &color);
        return color;
#endif
    }

    void Texture::clear(Color color) {
#ifndef TRI_USE_GL_4_5
        if ((int)width < 0 || (int)height < 0) {
            return;
        }
        std::vector<Color> colors;
        colors.resize(width * height, color);
        bind(0);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, colors.data());
#else
        glClearTexImage(id, 0, GL_RGBA, GL_UNSIGNED_BYTE, &color);
#endif
    }

}
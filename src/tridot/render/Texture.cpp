//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Texture.h"
#include "tridot/core/Log.h"
#include <GL/glew.h>

namespace tridot {

    Texture::Texture() {
        id = 0;
        slot = -1;

        width = 0;
        height = 0;
        channels = 0;

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

    void bindTexture(uint32_t id, int32_t slot){
        static int32_t currentIds[128];
        if(slot >= 0 && currentIds[slot] != id){
            glBindTextureUnit(slot, id);
            currentIds[slot] = id;
        }
    }

    void Texture::bind(int32_t slot) {
        bindTexture(id, slot);
        this->slot = slot;
    }

    void Texture::unbind() {
        bindTexture(id, slot);
        slot = -1;
    }

    uint32_t Texture::getId() {
        return id;
    }

    void Texture::setMagMin(bool magNearest, bool minNearest) {
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, magNearest ? GL_NEAREST : GL_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, minNearest ? GL_NEAREST : GL_LINEAR);
        this->magNearest = magNearest;
        this->minNearest = minNearest;
    }

    void Texture::setWrap(bool sRepeat, bool tRepeat) {
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, sRepeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, tRepeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
        this->sRepeat = sRepeat;
        this->tRepeat = tRepeat;
    }

    void Texture::create(uint32_t width, uint32_t height, TextureFormat format) {
        if(id == 0){
            glCreateTextures(GL_TEXTURE_2D, 1, &id);
            Log::trace("created texture ", id);
        }

        glTextureStorage2D(id, 1, internalEnum(format), width, height);

        this->width = width;
        this->height = height;
        this->channels = internalEnumSize(format) / 8;

        setMagMin(magNearest, minNearest);
        setWrap(sRepeat, tRepeat);
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
        glTextureSubImage2D(id, 0, 0, 0, image.getWidth(), image.getHeight(), dataFormat, GL_UNSIGNED_BYTE, image.getData());
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

}
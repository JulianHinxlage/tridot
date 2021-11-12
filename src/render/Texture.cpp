//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Texture.h"
#include "core/core.h"
#include <GL/glew.h>
#include <algorithm>

namespace tri {

    Texture::Texture() {
        id = 0;
        slot = -1;

        width = 0;
        height = 0;
        channels = 0;
        mipCount = 0;
        type = TEXTURE_2D;

        magNearest = true;
        minNearest = false;
        sRepeat = true;
        tRepeat = true;
    }

    Texture::~Texture() {
        if(id != 0){
            glDeleteTextures(1, &id);
            id = 0;
        }
    }

    void bindTexture(uint32_t id, TextureType type) {
        static int32_t currentId;
        if (currentId != id) {
            glBindTexture(internalEnum(type), id);
            currentId = id;
        }
    }

    void bindTexture(uint32_t id, int32_t slot, TextureType type){
        static int32_t currentIds[128];
        if(slot >= 0 && currentIds[slot] != id){
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(internalEnum(type), id);
            currentIds[slot] = id;
        }
    }

    void Texture::bind(int32_t slot) {
        bindTexture(id, slot, type);
        this->slot = slot;
    }

    void Texture::unbind() {
        bindTexture(0, slot, type);
        slot = -1;
    }

    uint32_t Texture::getId() {
        return id;
    }

    void Texture::setMagMin(bool magNearest, bool minNearest) {
        bindTexture(id, type);
        glTexParameteri(internalEnum(type), GL_TEXTURE_MAG_FILTER, magNearest ? GL_NEAREST : GL_LINEAR);
        if(mipCount > 1){
            glTexParameteri(internalEnum(type), GL_TEXTURE_MIN_FILTER, minNearest ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
        }else{
            glTexParameteri(internalEnum(type), GL_TEXTURE_MIN_FILTER, minNearest ? GL_NEAREST : GL_LINEAR);
        }
        this->magNearest = magNearest;
        this->minNearest = minNearest;
    }

    void Texture::setWrap(bool sRepeat, bool tRepeat) {
        bindTexture(id, type);
        glTexParameteri(internalEnum(type), GL_TEXTURE_WRAP_S, sRepeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
        glTexParameteri(internalEnum(type), GL_TEXTURE_WRAP_T, tRepeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
        this->sRepeat = sRepeat;
        this->tRepeat = tRepeat;
    }

    void Texture::setBorderColor(Color color) {
        bindTexture(id, type);
        glm::vec4 c = color.vec();
        glTexParameteri(internalEnum(type), GL_TEXTURE_WRAP_S, sRepeat ? GL_REPEAT : GL_CLAMP_TO_BORDER);
        glTexParameteri(internalEnum(type), GL_TEXTURE_WRAP_T, tRepeat ? GL_REPEAT : GL_CLAMP_TO_BORDER);
        glTexParameterfv(internalEnum(type), GL_TEXTURE_BORDER_COLOR, (float*)&c);
    }

    void Texture::create(uint32_t width, uint32_t height, TextureFormat format, bool enableMipMapping) {
        if(id != 0){
            glDeleteTextures(1, &id);
            glGenTextures(1, &id);
        }
        if(id == 0){
            glGenTextures(1, &id);
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
        glTexStorage2D(internalEnum(type), mipCount, internalEnum(format), width, height);
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
            env->console->error("only 8 bits per channel are supported");
        }

        create(image.getWidth(), image.getHeight(), format);
        glTexSubImage2D(internalEnum(type), 0, 0, 0, image.getWidth(), image.getHeight(), dataFormat, GL_UNSIGNED_BYTE, image.getData());
        if(mipCount > 1){
            glGenerateMipmap(internalEnum(type));
        }
        return true;
    }

    bool Texture::load(const std::string &file) {
        return image.load(file);
    }

    bool Texture::loadActivate() {
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
        glGetTexImage(internalEnum(type), 0, GL_RGBA, GL_UNSIGNED_BYTE, colors.data());
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
        glTexSubImage2D(internalEnum(type), 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, colors.data());
#else
        glClearTexImage(id, 0, GL_RGBA, GL_UNSIGNED_BYTE, &color);
#endif
    }

    void Texture::setCubeMap(bool enableMipMapping) {
        if(type != TEXTURE_CUBE_MAP && id != 0 && width != 0 && height != 0) {
            bind(0);
            int x = width / 4;
            int y = height / 3;
            int xs[] = {2, 0, 3, 1, 1, 1};
            int ys[] = {1, 1, 1, 1, 0, 2};
            int rs[] = {1, 3, 2, 0, 0, 2};

            std::vector<Color> colors[6];
            for(int i = 0; i < 6; i++) {
                colors[i].resize(x * y);
                glGetTextureSubImage(id, 0, xs[i] * x, ys[i] * y, 0, x, y, 1, GL_RGBA, GL_UNSIGNED_BYTE, sizeof(Color) * colors[i].size(), colors[i].data());
                std::vector<Color> tmp;
                tmp.resize(x * y);
                for(int j = 0; j < rs[i]; j++) {
                    for(int ix = 0; ix < x; ix++) {
                        for(int iy = 0; iy < y; iy++) {
                            tmp[ix + iy * x] = colors[i][(y - 1 - iy) + ix * x];
                        }
                    }
                    colors[i] = tmp;
                }
            }


            if(id != 0){
                glDeleteTextures(1, &id);
                glGenTextures(1, &id);
            }
            if(id == 0){
                glGenTextures(1, &id);
            }

            type = TEXTURE_CUBE_MAP;
            width = x;
            height = y;
            channels = internalEnumSize(format) / 8;


            bind(0);

            setMagMin(true, true);
            setWrap(false, false);

            mipCount = std::max(1, (int)std::log2(std::min(width, height)) - 1);
            if(!enableMipMapping) {
                mipCount = 1;
            }

            //for(int i = 0; i < 6; i++) {
            //    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalEnum(format), x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, colors[i].data());
            //}

            //mipCount = std::max(1, (int)std::log2(std::min(width, height)) - 1);
            //glTexStorage2D(internalEnum(type), mipCount, internalEnum(format), width, height);

            for(int i = 0; i < 6; i++) {
                glTexParameteri(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, GL_TEXTURE_MAX_LEVEL, mipCount);
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalEnum(format), x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, colors[i].data());
                //if(mipCount > 1){
                //    glGenerateMipmap(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
                //}


                //glTexStorage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mipCount, internalEnum(format), width, height);
                //glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, image.getWidth(), image.getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, image.getData());

                //if(mipCount > 1){
                //    glGenerateMipmap(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
                //}
            }
            if(mipCount > 1){
                glGenerateMipmap(internalEnum(type));
            }
        }
    }

    TextureType Texture::getType() {
        return type;
    }

}
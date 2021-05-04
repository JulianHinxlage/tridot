//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "FrameBuffer.h"
#include "tridot/core/Log.h"
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <algorithm>

namespace tridot {

    FrameBuffer::FrameBuffer() {
        id = 0;
        width = 0;
        height = 0;
    }

    FrameBuffer::~FrameBuffer() {
        if(id != 0){
            glDeleteFramebuffers(1, &id);
            Log::trace("deleted frame buffer ", id);
            id = 0;
        }
    }

    static uint32_t currentFrameBufferId = 0;
    void bindFrameBuffer(uint32_t id){
        if(currentFrameBufferId != id){
            glBindFramebuffer(GL_FRAMEBUFFER, id);
            currentFrameBufferId = id;
        }
    }

    void FrameBuffer::bind() const {
        if(currentFrameBufferId != id) {
            bindFrameBuffer(id);
            glViewport(0, 0, width, height);

            std::vector<GLenum> drawBuffers;
            for(auto &texture : textures){
                if(texture.second.get() != nullptr) {
                    if (texture.first >= COLOR && texture.first < COLOR + 16) {
                        drawBuffers.push_back(internalEnum((TextureAttachment)texture.first));
                    }
                }
            }
            std::sort(drawBuffers.begin(), drawBuffers.end());
            glDrawBuffers(drawBuffers.size(), drawBuffers.data());
        }
    }

    void FrameBuffer::unbind() {
        if(currentFrameBufferId != 0) {
            bindFrameBuffer(0);
            GLFWwindow *window = glfwGetCurrentContext();
            if (window) {
                int width = 0;
                int height = 0;
                glfwGetWindowSize(window, &width, &height);
                glViewport(0, 0, width, height);
            }
        }
    }

    uint32_t FrameBuffer::getId() const {
        return id;
    }

    glm::vec2 FrameBuffer::getSize() {
        return glm::vec2(width, height);
    }

    Ref<Texture> FrameBuffer::getTexture(TextureAttachment attachment) {
        auto entry = textures.find((uint32_t)attachment);
        if(entry != textures.end()){
            return entry->second;
        }else{
            return nullptr;
        }
    }

    Ref<Texture> FrameBuffer::setTexture(TextureAttachment attachment, const Ref<Texture> &texture) {
        if(id == 0){
            glGenFramebuffers(1, &id);
            Log::trace("created frame buffer ", id);
        }
        bindFrameBuffer(id);
        glFramebufferTexture2D(GL_FRAMEBUFFER, internalEnum(attachment), GL_TEXTURE_2D, texture->getId(), 0);

        auto entry = textures.find((uint32_t)attachment);
        if(entry != textures.end()){
            entry->second = texture;
            return entry->second;
        }else{
            textures[(uint32_t)attachment] = texture;
            return textures[(uint32_t)attachment];
        }
    }

    Ref<Texture> FrameBuffer::setTexture(TextureAttachment attachment) {
        Ref<Texture> texture;
        switch (attachment) {
            case DEPTH:
                texture = Ref<Texture>::make();
                texture->create(width, height, TextureFormat::DEPTH24, false);
                break;
            case STENCIL:
                texture = Ref<Texture>::make();
                texture->create(width, height, TextureFormat::DEPTH24STENCIL8, false);
                break;
            case COLOR:
                texture = Ref<Texture>::make();
                texture->create(width, height, TextureFormat::RGBA8, false);
                break;
            default:
                if(attachment > COLOR && attachment < COLOR + 16){
                    texture = Ref<Texture>::make();
                    texture->create(width, height, TextureFormat::RGBA8, false);
                }
                break;
        }
        return setTexture(attachment, texture);
    }

    void FrameBuffer::resize(uint32_t width, uint32_t height) {
        bindFrameBuffer(id);
        for(auto &texture : textures){
            if(texture.second.get() != nullptr){
                if(texture.second->getWidth() != width || texture.second->getHeight() != height){
                    texture.second->create(width, height, texture.second->getFormat(), false);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, internalEnum((TextureAttachment)texture.first), GL_TEXTURE_2D, texture.second->getId(), 0);
                }
            }
        }
        this->width = width;
        this->height = height;
        unbind();
        bind();
    }

    void FrameBuffer::clear(Color color) {
        uint32_t current = currentFrameBufferId;
        bindFrameBuffer(id);
        glm::vec4 c = color.vec();
        glClearColor(c.r, c.g, c.b, c.a);
        uint32_t mask = 0;
        for(auto &texture : textures){
            if(texture.second.get() != nullptr) {
                if (texture.first == DEPTH) {
                    mask |= GL_DEPTH_BUFFER_BIT;
                }
                if (texture.first == STENCIL) {
                    mask |= GL_STENCIL_BUFFER_BIT;
                }
                if (texture.first >= COLOR && texture.first < COLOR + 16) {
                    mask |= GL_COLOR_BUFFER_BIT;
                }
            }
        }
        glClear(mask);
        bindFrameBuffer(current);
    }

}
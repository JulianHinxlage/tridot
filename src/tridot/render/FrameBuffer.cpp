//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "FrameBuffer.h"
#include "tridot/core/Log.h"
#include "GL/glew.h"
#include "GLFW/glfw3.h"

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
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);
            currentFrameBufferId = id;
        }
    }

    void FrameBuffer::bind() const {
        if(currentFrameBufferId != id) {
            bindFrameBuffer(id);
            glViewport(0, 0, width, height);
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
            glCreateFramebuffers(1, &id);
            Log::trace("created frame buffer ", id);
        }
        glNamedFramebufferTexture(id, internalEnum(attachment), texture->getId(), 0);
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
                texture->create(width, height, TextureFormat::DEPTH24);
                break;
            case STENCIL:
                texture = Ref<Texture>::make();
                texture->create(width, height, TextureFormat::DEPTH24STENCIL8);
                break;
            case COLOR:
                texture = Ref<Texture>::make();
                texture->create(width, height, TextureFormat::RGBA8);
                break;
            default:
                if(attachment > COLOR && attachment < COLOR + 16){
                    texture = Ref<Texture>::make();
                    texture->create(width, height, TextureFormat::RGBA8);
                }
                break;
        }
        return setTexture(attachment, texture);
    }

    void FrameBuffer::resize(uint32_t width, uint32_t height) {
        for(auto &texture : textures){
            if(texture.second.get() != nullptr){
                if(texture.second->getWidth() != width || texture.second->getHeight() != height){
                    texture.second->create(width, height, texture.second->getFormat());
                    glNamedFramebufferTexture(id, internalEnum((TextureAttachment)texture.first), texture.second->getId(), 0);
                }
            }
        }
        this->width = width;
        this->height = height;
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
//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "FrameBuffer.h"
#include "core/core.h"
#include "RenderContext.h"
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <algorithm>

namespace tri {

    FrameBuffer::FrameBuffer() {
        id = 0;
        width = 0;
        height = 0;
    }

    FrameBuffer::FrameBuffer(const FrameBuffer &frameBuffer) {
        id = 0;
        width = frameBuffer.width;
        height = frameBuffer.height;
        for(auto &attachment : frameBuffer.attachments){
            setAttachment(attachment.second.spec);
        }
    }

    FrameBuffer::FrameBuffer(uint32_t width, uint32_t height, std::vector<FrameBufferAttachmentSpec> specs) {
        id = 0;
        this->width = 0;
        this->height = 0;
        init(width, height, specs);
    }

    FrameBuffer::~FrameBuffer() {
        if(id != 0){
            glDeleteFramebuffers(1, &id);
            env->console->trace("deleted frame buffer ", id);
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
            for(auto &attachment : attachments){
                if(attachment.second.texture.get() != nullptr) {
                    if (attachment.first >= COLOR && attachment.first < COLOR + 16) {
                        if(attachment.second.texture->getWidth() != width || attachment.second.texture->getHeight() != height){
                            glViewport(0, 0, attachment.second.texture->getWidth(), attachment.second.texture->getHeight());
                        }
                        drawBuffers.push_back(internalEnum((TextureAttachment)attachment.first));
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
            GLFWwindow *window = (GLFWwindow*)RenderContext::get();
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

    void FrameBuffer::init(uint32_t width, uint32_t height, std::vector<FrameBufferAttachmentSpec> specs) {
        this->width = width;
        this->height = height;
        for(auto &spec : specs){
            setAttachment(spec);
        }
    }

    Ref<Texture> FrameBuffer::getAttachment(TextureAttachment attachment) {
        auto entry = attachments.find((uint32_t)attachment);
        if(entry != attachments.end()){
            return entry->second.texture;
        }else{
            return nullptr;
        }
    }

    Ref<Texture> FrameBuffer::setAttachment(FrameBufferAttachmentSpec spec, const Ref<Texture> &texture) {
        if(id == 0){
            glGenFramebuffers(1, &id);
            env->console->trace("created frame buffer ", id);
        }
        bindFrameBuffer(id);
        glFramebufferTexture2D(GL_FRAMEBUFFER, internalEnum(spec.type), GL_TEXTURE_2D, texture->getId(), 0);

        auto entry = attachments.find((uint32_t)spec.type);
        if(entry != attachments.end()){
            entry->second.spec = spec;
            entry->second.texture = texture;
            return entry->second.texture;
        }else{
            attachments[(uint32_t)spec.type] = {texture, spec};
            return attachments[(uint32_t)spec.type].texture;
        }
    }

    Ref<Texture> FrameBuffer::setAttachment(FrameBufferAttachmentSpec spec) {
        auto entry = attachments.find((uint32_t)spec.type);
        if(entry != attachments.end()){
            entry->second.spec = spec;
            return entry->second.texture;
        }

        Ref<Texture> texture;
        switch (spec.type) {
            case DEPTH:
                texture = Ref<Texture>::make();
                texture->create(width, height, TextureFormat::DEPTH24, false);
                break;
            case STENCIL:
                texture = Ref<Texture>::make();
                texture->create(width, height, TextureFormat::DEPTH24STENCIL8, false);
                break;
            default:
                if(spec.type >= COLOR && spec.type < COLOR + 16){
                    texture = Ref<Texture>::make();
                    texture->create(width, height, TextureFormat::RGBA8, false);
                }
                break;
        }
        return setAttachment(spec, texture);
    }

    void FrameBuffer::resize(uint32_t width, uint32_t height) {
        bindFrameBuffer(id);
        for(auto &attachment : attachments){
            auto &texture = attachment.second.texture;
            if(texture.get() != nullptr){
                if(attachment.second.spec.resize){
                    int x = width * attachment.second.spec.resizeFactor.x;
                    int y = height * attachment.second.spec.resizeFactor.y;
                    if(texture->getWidth() != x || texture->getHeight() != y){
                        texture->create(x, y, texture->getFormat(), false);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, internalEnum((TextureAttachment)attachment.first), GL_TEXTURE_2D, texture->getId(), 0);
                    }
                }
            }
        }
        this->width = width;
        this->height = height;
        unbind();
        bind();
    }

    void FrameBuffer::clear() {
        uint32_t current = currentFrameBufferId;
        bindFrameBuffer(id);
        for(auto &attachment : attachments){
            clear((TextureAttachment)attachment.first);
        }
        bindFrameBuffer(current);
    }

    void FrameBuffer::clear(TextureAttachment attachment) {
        auto entry = attachments.find((uint32_t)attachment);
        if(entry != attachments.end()){
            if(entry->second.spec.clear){
                int bit = 0;
                if (entry->first == DEPTH) {
                    bit |= GL_DEPTH_BUFFER_BIT;
                }else if (entry->first == STENCIL) {
                    bit |= GL_STENCIL_BUFFER_BIT;
                }else if (entry->first >= COLOR && entry->first < COLOR + 16) {
                    bit |= GL_COLOR_BUFFER_BIT;
                }

                glm::vec4 color = entry->second.spec.clearColor.vec();
                glDrawBuffer(internalEnum((TextureAttachment)entry->first));
                glClearColor(color.r, color.g, color.b, color.a);
                glClear(bit);
            }
        }
    }

}
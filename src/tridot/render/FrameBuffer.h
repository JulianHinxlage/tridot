//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_FRAMEBUFFER_H
#define TRIDOT_FRAMEBUFFER_H

#include "Texture.h"
#include "tridot/core/Ref.h"
#include <unordered_map>

namespace tridot {

    class FrameBufferAttachmentSpec {
    public:
        TextureAttachment type = COLOR;
        Color clearColor = Color::white;
        glm::vec2 resizeFactor = {1, 1};
        bool clear = true;
        bool resize = true;
    };

    class FrameBuffer {
    public:
        FrameBuffer();
        FrameBuffer(const FrameBuffer &frameBuffer);
        FrameBuffer(uint32_t width, uint32_t height, std::vector<FrameBufferAttachmentSpec> specs);
        ~FrameBuffer();

        void bind() const;
        static void unbind();
        uint32_t getId() const;
        glm::vec2 getSize();

        void init(uint32_t width, uint32_t height, std::vector<FrameBufferAttachmentSpec> specs);

        Ref<Texture> getAttachment(TextureAttachment attachment);
        Ref<Texture> setAttachment(FrameBufferAttachmentSpec spec, const Ref<Texture> &texture);
        Ref<Texture> setAttachment(FrameBufferAttachmentSpec spec);

        void resize(uint32_t width, uint32_t height);
        void clear();
        void clear(TextureAttachment attachment);
    private:
        uint32_t id;
        uint32_t width;
        uint32_t height;

        class Attachment{
        public:
            Ref<Texture> texture;
            FrameBufferAttachmentSpec spec;
        };

        std::unordered_map<uint32_t, Attachment> attachments;
    };

}

#endif //TRIDOT_FRAMEBUFFER_H

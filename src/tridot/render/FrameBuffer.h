//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_FRAMEBUFFER_H
#define TRIDOT_FRAMEBUFFER_H

#include "Texture.h"
#include "tridot/core/Ref.h"
#include <unordered_map>

namespace tridot {

    class FrameBuffer {
    public:
        FrameBuffer();
        ~FrameBuffer();

        void bind() const;
        static void unbind();
        uint32_t getId() const;
        glm::vec2 getSize();

        Ref<Texture> getTexture(TextureAttachment attachment);
        Ref<Texture> setTexture(TextureAttachment attachment, const Ref<Texture> &texture);
        Ref<Texture> setTexture(TextureAttachment attachment);

        void resize(uint32_t width, uint32_t height);
        void clear(Color color);
    private:
        uint32_t id;
        uint32_t width;
        uint32_t height;
        std::unordered_map<uint32_t, Ref<Texture>> textures;
    };

}

#endif //TRIDOT_FRAMEBUFFER_H

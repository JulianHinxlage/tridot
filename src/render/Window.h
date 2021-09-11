//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "Color.h"
#include "glm/glm.hpp"
#include <string>

namespace tri {

    class Window : public System {
    public:
        Window();
        Window(int width, int height, const std::string &title);
        ~Window();
        void init(int width, int height, const std::string &title, bool fullscreen = false);
        void update() override;
        bool isOpen();
        void close();
        void bind();
        void unbind();

        const glm::vec2 &getSize() const;
        const glm::vec2 &getPosition() const;
        const Color &getBackgroundColor() const;
        bool getVSync() const;
        float getAspectRatio() const;
        void *getContext() const;
        void setSize(const glm::vec2 &size);
        void setPosition(const glm::vec2 &position);
        void setBackgroundColor(const Color &color);
        void setTitle(const std::string &title);
        void setVSync(bool enabled);
    private:
        bool vsync;
        Color backgroundColor;
        glm::vec2 size;
        glm::vec2 position;
        void *context;

        void clear();
    };

}


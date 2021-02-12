//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_WINDOW_H
#define TRIDOT_WINDOW_H

#include "glm/glm.hpp"

namespace tridot {

    class Window {
    public:
        Window();
        Window(int width, int height, const char *title);
        ~Window();
        void init(int width, int height, const char *title);
        void update();
        bool isOpen();
        void close();
        void bind();
        void unbind();

        const glm::vec2 &getSize() const;
        const glm::vec2 &getPosition() const;
        const glm::vec4 &getBackgroundColor() const;
        bool getVSync() const;
        float getAspectRatio() const;
        void *getContext() const;
        void setSize(const glm::vec2 &size);
        void setPosition(const glm::vec2 &position);
        void setBackgroundColor(const glm::vec4 &color);
        void setVSync(bool enabled);
    private:
        bool vsync;
        glm::vec4 backgroundColor;
        glm::vec2 size;
        glm::vec2 position;
        void *context;

        void clear();
    };

}

#endif //TRIDOT_WINDOW_H

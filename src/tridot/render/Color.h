//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_COLOR_H
#define TRIDOT_COLOR_H

#include <cstdint>
#include <glm/glm.hpp>

namespace tridot {

    class Color {
    public:
        union{
            uint32_t value;
            struct{
                uint8_t r;
                uint8_t g;
                uint8_t b;
                uint8_t a;
            };
        };

        Color(uint32_t value = 0xffffffff);
        Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
        Color(const glm::vec4 &color);
        glm::vec4 vec() const;
        bool operator==(const Color &color) const;
        bool operator!=(const Color &color) const;
        Color operator*(float rhs) const;
        Color operator/(float rhs) const;
        Color operator*(Color rhs) const;
        Color operator+(Color rhs) const;
        Color operator-(Color rhs) const;

        static const Color white;
        static const Color black;
        static const Color red;
        static const Color green;
        static const Color blue;
        static const Color transparent;
    };

}

#endif //TRIDOT_COLOR_H
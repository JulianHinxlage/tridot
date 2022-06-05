//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/config.h"
#include <cstdint>
#include <glm/glm.hpp>

namespace tri {

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
        Color(const glm::vec3 &color);
        glm::vec4 vec() const;
        bool operator==(const Color &color) const;
        bool operator!=(const Color &color) const;
        Color operator*(float rhs) const;
        Color operator/(float rhs) const;
        Color operator*(Color rhs) const;
        Color operator+(Color rhs) const;
        Color operator-(Color rhs) const;
    };

    namespace color {
        static const Color white = 0xffffffff;
        static const Color black = 0xff000000;
        static const Color red = 0xff0000ff;
        static const Color green = 0xff00ff00;
        static const Color blue = 0xffff0000;
        static const Color transparent = 0x00ffffff;
    }

}


//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/config.h"
#include <cstdint>
#include <glm/glm.hpp>

namespace tri {

    class TRI_API Color {
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

        static const Color white;
        static const Color black;
        static const Color red;
        static const Color green;
        static const Color blue;
        static const Color transparent;
    };

}


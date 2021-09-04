//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Color.h"

namespace tridot {

    const Color Color::white = 0xffffffff;
    const Color Color::black = 0xff000000;
    const Color Color::red = 0xff0000ff;
    const Color Color::green = 0xff00ff00;
    const Color Color::blue = 0xffff0000;
    const Color Color::transparent = 0x00ffffff;
    
    Color::Color(uint32_t value) {
        this->value = value;
    }

    Color::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        this->r = r;
        this->g = g;
        this->b = b;
        this->a = a;
    }

    Color::Color(const glm::vec4 &color) {
        this->r = color.r * 255;
        this->g = color.g * 255;
        this->b = color.b * 255;
        this->a = color.a * 255;
    }

    Color::Color(const glm::vec3 &color) {
        this->r = color.r * 255;
        this->g = color.g * 255;
        this->b = color.b * 255;
        this->a = 255;
    }

    glm::vec4 Color::vec() const {
        return glm::vec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f);
    }

    bool Color::operator==(const Color &color) const {
        return value == color.value;
    }

    bool Color::operator!=(const Color &color) const {
        return value != color.value;
    }

    Color Color::operator*(float rhs) const {
        return vec() * rhs;
    }

    Color Color::operator/(float rhs) const {
        return vec() / rhs;
    }

    Color Color::operator*(Color rhs) const {
        return vec() * rhs.vec();
    }

    Color Color::operator+(Color rhs) const {
        return vec() + rhs.vec();
    }

    Color Color::operator-(Color rhs) const {
        return vec() - rhs.vec();
    }

}

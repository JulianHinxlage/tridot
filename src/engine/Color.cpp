//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Color.h"

namespace tri {
    
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
        this->r = glm::clamp(color.r, 0.0f, 1.0f) * 255;
        this->g = glm::clamp(color.g, 0.0f, 1.0f) * 255;
        this->b = glm::clamp(color.b, 0.0f, 1.0f) * 255;
        this->a = glm::clamp(color.a, 0.0f, 1.0f) * 255;
    }

    Color::Color(const glm::vec3 &color) {
        this->r = glm::clamp(color.r, 0.0f, 1.0f) * 255;
        this->g = glm::clamp(color.g, 0.0f, 1.0f) * 255;
        this->b = glm::clamp(color.b, 0.0f, 1.0f) * 255;
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

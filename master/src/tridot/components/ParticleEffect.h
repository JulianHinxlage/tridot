//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "tridot/render/Texture.h"
#include "tridot/render/Material.h"
#include "tridot/util/Ref.h"

namespace tridot {

    class ParticleEffect {
    public:
        Ref<Texture> texture = nullptr;
        bool active = true;
        int particlesPerTrigger = 0;
        float particlesPerSecond = 0;
        float particlesPerDistance = 0;
        float lifeTime = 1;
        float lifeTimeVariance = 0;
        float fadeOutTime = 0.1;
        float fadeInTime = 0.1;

        glm::vec3 velocity = {0, 0, 0};
        glm::vec3 velocityVariance = {0, 0, 0};
        glm::vec3 positionVariance = {0, 0, 0};

        Color beginColor = Color::white;
        Color endColor = Color::white;
        Color beginColorVariance = Color::black;
        Color endColorVariance = Color::black;

        float rotation = 0;
        float rotationVariance = 360;
        float angular = 0;
        float angularVariance = 0;

        glm::vec3 beginScale = {1, 1, 1};
        glm::vec3 endScale = {1, 1, 1};
        glm::vec3 beginScaleVariance = {0, 0, 0};
        glm::vec3 endScaleVariance = {0, 0, 0};
    };

    class Particle{
    public:
        Ref<Texture> texture = nullptr;
        float spawnTime = 0;
        float lifeTime = 0;
        float fadeOutTime = 0;
        float fadeInTime = 0;

        Color beginColor = Color::white;
        Color endColor = Color::white;
        glm::vec3 beginScale = {0, 0, 0};
        glm::vec3 endScale = {0, 0, 0};
        float beginRotation = 0;
        float endRotation = 0;
        glm::vec3 beginPosition = {0, 0, 0};
        glm::vec3 endPosition = {0 ,0 ,0};
    };

}


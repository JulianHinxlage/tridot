//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "engine/Color.h"
#include <glm/glm.hpp>

namespace tri {

    //insatnce data for shader
    struct InstanceData {
    public:
        glm::mat4 transform;
        Color color;
        float materialIndex;
        uint32_t id;
    };

    //material data for shader
    class MaterialData {
    public:
        glm::vec4 color;
        int mapping;
        float roughness;
        float metallic;
        float normalMapFactor;
        float emissive;

        int texture;
        int normalMap;
        int roughnessMap;
        int metallicMap;
        int ambientOcclusionMap;
        int displacementMap;
        int align1;

        glm::vec2 textureOffset;
        glm::vec2 textureScale;

        glm::vec2 normalMapOffset;
        glm::vec2 normalMapScale;

        glm::vec2 roughnessMapOffset;
        glm::vec2 roughnessMapScale;

        glm::vec2 metallicMapOffset;
        glm::vec2 metallicMapScale;

        glm::vec2 ambientOcclusionMapOffset;
        glm::vec2 ambientOcclusionMapScale;

        glm::vec2 displacementMapOffset;
        glm::vec2 displacementMapScale;
    };

    //light data for shader
    class LightData {
    public:
        glm::vec3 position;
        int align1;
        glm::vec3 direction;
        int align2;
        glm::vec3 color;
        int align3;
        int type;
        float intensity;
        float radius;
        int shadowMapIndex;
        glm::mat4 projection;
    };

    //environment data for shader
    class EnvironmentData {
    public:
        glm::mat4 projection;
        glm::mat4 view;
        glm::mat4 viewProjection;
        glm::vec3 eyePosition;
        int align1;
        int lightCount;
        float environmentMapIntensity;
        int radianceMapIndex;
        int irradianceMapIndex;
    };

}

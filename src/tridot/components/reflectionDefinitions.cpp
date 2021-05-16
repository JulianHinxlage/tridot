//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/ecs/Reflection.h"
#include "Transform.h"
#include "Tag.h"
#include "RenderComponent.h"
#include "PostProcessingEffect.h"
#include "ComponentCache.h"
#include "tridot/render/Camera.h"
#include "tridot/render/Light.h"
#include "tridot/engine/Physics.h"
#include <glm/glm.hpp>

namespace tridot {

    TRI_REFLECT_TYPE(float)
    TRI_REFLECT_TYPE(bool)
    TRI_REFLECT_TYPE(int)

    TRI_REFLECT_TYPE_NAME(glm::vec2, vec2)
    TRI_REFLECT_MEMBER2(glm::vec2, x, y)

    TRI_REFLECT_TYPE_NAME(glm::vec3, vec3)
    TRI_REFLECT_MEMBER3(glm::vec3, x, y, z)

    TRI_REFLECT_TYPE_NAME(glm::vec4, vec4)
    TRI_REFLECT_MEMBER4(glm::vec4, x, y, z, w)

    TRI_REFLECT_TYPE(Tag)
    TRI_REFLECT_MEMBER(Tag, tag)

    TRI_REFLECT_TYPE(uuid)
    TRI_REFLECT_MEMBER2(uuid, v1, v2)

    TRI_REFLECT_TYPE(Transform)
    TRI_REFLECT_MEMBER4(Transform, position, scale, rotation, parent.id)

    TRI_REFLECT_TYPE(RenderComponent)
    TRI_REFLECT_MEMBER3(RenderComponent, mesh, material, color)

    TRI_REFLECT_TYPE(PostProcessingEffect)
    TRI_REFLECT_MEMBER2(PostProcessingEffect, shader, frameBuffer)

    TRI_REFLECT_TYPE(Collider)
    TRI_REFLECT_MEMBER2(Collider, scale, type)

    TRI_REFLECT_TYPE(RigidBody)
    TRI_REFLECT_MEMBER8(RigidBody, velocity, angular, mass, friction, restitution, linearDamping, angularDamping, enablePhysics)

    TRI_REFLECT_TYPE(Light)
    TRI_REFLECT_MEMBER3(Light, color, intensity, type)

    TRI_REFLECT_TYPE(PerspectiveCamera)
    TRI_REFLECT_MEMBER9(PerspectiveCamera, position, forward, up, right, fieldOfView, aspectRatio, near, far, target)

    TRI_REFLECT_TYPE(OrthographicCamera)
    TRI_REFLECT_MEMBER7(OrthographicCamera, position, scale, up, right, rotation, aspectRatio, target)

    TRI_REFLECT_TYPE(Material)
    TRI_REFLECT_MEMBER10(Material,
         color,
         mapping,
         roughness,
         metallic,
         normalMapFactor,
         texture,
         normalMap,
         roughnessMap,
         metallicMap,
         textureOffset
    )
    TRI_REFLECT_MEMBER8(Material,
        textureScale,
        normalMapOffset,
        normalMapScale,
        roughnessMapOffset,
        roughnessMapScale,
        metallicMapOffset,
        metallicMapScale,
        shader
    )

    TRI_REFLECT_TYPE(ComponentCache)
    TRI_REFLECT_MEMBER(ComponentCache, data)

}
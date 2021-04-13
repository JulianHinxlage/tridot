//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/ecs/Reflection.h"
#include "Transform.h"
#include "Tag.h"
#include "RenderComponent.h"
#include "ComponentCache.h"
#include "tridot/render/Camera.h"
#include "tridot/render/Light.h"
#include "tridot/engine/Physics.h"
#include <glm/glm.hpp>

namespace tridot {

    REFLECT_TYPE(float)
    REFLECT_TYPE(bool)
    REFLECT_TYPE(int)

    REFLECT_TYPE_NAME(glm::vec2, vec2)
    REFLECT_MEMBER2(glm::vec2, x, y)

    REFLECT_TYPE_NAME(glm::vec3, vec3)
    REFLECT_MEMBER3(glm::vec3, x, y, z)

    REFLECT_TYPE_NAME(glm::vec4, vec4)
    REFLECT_MEMBER4(glm::vec4, x, y, z, w)

    REFLECT_TYPE(Tag)
    REFLECT_MEMBER(Tag, tag)

    REFLECT_TYPE(uuid)

    REFLECT_TYPE(Transform)
    REFLECT_MEMBER3(Transform, position, scale, rotation)

    REFLECT_TYPE(RenderComponent)
    REFLECT_MEMBER3(RenderComponent, mesh, material, color)

    REFLECT_TYPE(Collider)
    REFLECT_MEMBER2(Collider, scale, type)

    REFLECT_TYPE(RigidBody)
    REFLECT_MEMBER8(RigidBody, velocity, angular, mass, friction, restitution, linearDamping, angularDamping, enablePhysics)

    REFLECT_TYPE(Light)
    REFLECT_MEMBER3(Light, color, intensity, type)

    REFLECT_TYPE(PerspectiveCamera)
    REFLECT_MEMBER9(PerspectiveCamera, position, forward, up, right, fieldOfView, aspectRatio, near, far, target)

    REFLECT_TYPE(OrthographicCamera)
    REFLECT_MEMBER7(OrthographicCamera, position, scale, up, right, rotation, aspectRatio, target)

    REFLECT_TYPE(Material)
    REFLECT_MEMBER10(Material,
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
    REFLECT_MEMBER8(Material,
        textureScale,
        normalMapOffset,
        normalMapScale,
        roughnessMapOffset,
        roughnessMapScale,
        metallicMapOffset,
        metallicMapScale,
        shader
    )

    REFLECT_TYPE(ComponentCache)
    REFLECT_MEMBER(ComponentCache, data)

}
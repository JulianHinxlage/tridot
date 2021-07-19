//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/entity/ComponentRegister.h"
#include "Transform.h"
#include "Tag.h"
#include "RenderComponent.h"
#include "PostProcessingEffect.h"
#include "ComponentCache.h"
#include "SkyBox.h"
#include "tridot/render/Camera.h"
#include "tridot/render/Light.h"
#include "tridot/engine/Physics.h"
#include <glm/glm.hpp>


namespace tridot {

    TRI_INIT_CALLBACK("type registering"){
        ComponentRegister::registerComponent<Tag, uuid, Transform, RenderComponent, PostProcessingEffect, PerspectiveCamera, OrthographicCamera, Light, RigidBody, Collider, ComponentCache, SkyBox>();
    }

    TRI_REGISTER_TYPE(float)
    TRI_REGISTER_TYPE(bool)
    TRI_REGISTER_TYPE(int)

    TRI_REGISTER_TYPE_NAME(glm::vec2, vec2)
    TRI_REGISTER_MEMBER2(glm::vec2, x, y)

    TRI_REGISTER_TYPE_NAME(glm::vec3, vec3)
    TRI_REGISTER_MEMBER3(glm::vec3, x, y, z)

    TRI_REGISTER_TYPE_NAME(glm::vec4, vec4)
    TRI_REGISTER_MEMBER4(glm::vec4, x, y, z, w)

    TRI_REGISTER_TYPE(Tag)
    TRI_REGISTER_MEMBER(Tag, tag)

    TRI_REGISTER_TYPE(uuid)
    TRI_REGISTER_MEMBER2(uuid, v1, v2)

    TRI_REGISTER_TYPE(Transform)
    TRI_REGISTER_MEMBER4(Transform, position, scale, rotation, parent.id)

    TRI_REGISTER_TYPE(RenderComponent)
    TRI_REGISTER_MEMBER3(RenderComponent, mesh, material, color)

    TRI_REGISTER_TYPE(PostProcessingEffect)
    TRI_REGISTER_MEMBER2(PostProcessingEffect, shader, frameBuffer)

    TRI_REGISTER_TYPE(Collider)
    TRI_REGISTER_MEMBER2(Collider, scale, type)

    TRI_REGISTER_TYPE(RigidBody)
    TRI_REGISTER_MEMBER8(RigidBody, velocity, angular, mass, friction, restitution, linearDamping, angularDamping, enablePhysics)

    TRI_REGISTER_TYPE(Light)
    TRI_REGISTER_MEMBER3(Light, color, intensity, type)

    TRI_REGISTER_TYPE(PerspectiveCamera)
    TRI_REGISTER_MEMBER5(PerspectiveCamera, fieldOfView, aspectRatio, near, far, target)

    TRI_REGISTER_TYPE(OrthographicCamera)
    TRI_REGISTER_MEMBER4(OrthographicCamera, scale, rotation, aspectRatio, target)

    TRI_REGISTER_TYPE(Material)
    TRI_REGISTER_MEMBER10(Material,
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
    TRI_REGISTER_MEMBER8(Material,
        textureScale,
        normalMapOffset,
        normalMapScale,
        roughnessMapOffset,
        roughnessMapScale,
        metallicMapOffset,
        metallicMapScale,
        shader
    )

    TRI_REGISTER_TYPE(ComponentCache)
    TRI_REGISTER_MEMBER(ComponentCache, data)

    TRI_REGISTER_TYPE(SkyBox)
    TRI_REGISTER_MEMBER4(SkyBox, texture, drawSkybox, useEnvironmentMap, intensity)

}
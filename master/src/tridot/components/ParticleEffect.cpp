//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "ParticleEffect.h"
#include "tridot/core/Environment.h"
#include "tridot/entity/ComponentRegister.h"
#include "tridot/engine/Scene.h"
#include "tridot/engine/Time.h"
#include "tridot/engine/Profiler.h"
#include "tridot/engine/ResourceManager.h"
#include "tridot/engine/Serializer.h"
#include "tridot/render/Window.h"
#include "tridot/render/Camera.h"
#include "tridot/render/PBRenderer.h"
#include "tridot/render/MeshRenderer.h"
#include "tridot/components/Transform.h"
#include "tridot/render/RenderContext.h"

namespace tridot {

    TRI_REGISTER_TYPE(ParticleEffect)
    TRI_REGISTER_MEMBER9(ParticleEffect, texture, active, particlesPerTrigger, particlesPerSecond, particlesPerDistance, lifeTime, lifeTimeVariance, fadeInTime, fadeOutTime)
    TRI_REGISTER_MEMBER8(ParticleEffect, velocity, velocityVariance, positionVariance, beginColor, endColor, beginColorVariance, endColorVariance, rotation)
    TRI_REGISTER_MEMBER7(ParticleEffect, rotationVariance, angular, angularVariance, beginScale, endScale, beginScaleVariance, endScaleVariance)

    TRI_INIT_CALLBACK("ParticleEffect"){
        ComponentRegister::registerComponent<ParticleEffect, Particle>();
        env->events->update.callbackOrder({"rendering", "Particle", "post processing"});
    }

    TRI_REGISTER_TYPE(Particle)
    TRI_REGISTER_MEMBER10(Particle, texture, spawnTime, lifeTime, fadeInTime, fadeOutTime, beginColor, endColor, beginScale, endScale, beginRotation)
    TRI_REGISTER_MEMBER3(Particle, endRotation, beginPosition, endPosition)

    float randVar(){
        return ((float)std::rand() / (float)RAND_MAX) - 0.5f;
    }

    glm::vec3 randVar3(){
        return {randVar(), randVar(), randVar()};
    }

    glm::vec4 randVar4(){
        return {randVar(), randVar(), randVar(), randVar()};
    }

    template<typename T>
    T lerp(T a, T b, float t){
        return a + (b - a) * t;
    }

    TRI_UPDATE_CALLBACK("ParticleEffect"){
        env->scene->view<ParticleEffect, Transform>().each([](ParticleEffect &effect, Transform &transform){
            if(effect.active){
                int count = env->time->deltaTicks(1.0f / effect.particlesPerSecond);
                for(int i = 0; i < count; i++){
                    EntityId id = env->scene->create(NoSerial());
                    Particle &particle = env->scene->add<Particle>(id);

                    particle.texture = effect.texture;
                    particle.spawnTime = env->time->time;
                    particle.lifeTime = effect.lifeTime + randVar() * effect.lifeTimeVariance;
                    particle.fadeInTime = effect.fadeInTime;
                    particle.fadeOutTime = effect.fadeOutTime;
                    particle.beginColor = effect.beginColor.vec() + randVar4() * effect.beginColorVariance.vec();
                    particle.endColor = effect.endColor.vec() + randVar4() * effect.endColorVariance.vec();
                    particle.beginScale = effect.beginScale + randVar3() * effect.beginScaleVariance;
                    particle.endScale = effect.endScale + randVar3() * effect.endScaleVariance;
                    particle.beginRotation = effect.rotation + randVar() * effect.rotationVariance;
                    particle.endRotation = particle.beginRotation + (effect.angular + randVar() * effect.angularVariance) * particle.lifeTime;
                    particle.beginPosition = randVar3() * effect.positionVariance;
                    particle.endPosition = particle.beginPosition + (effect.velocity + randVar3() * effect.velocityVariance) * particle.lifeTime;

                    glm::mat4 matrix = transform.getMatrix();
                    particle.beginPosition = matrix * glm::vec4(particle.beginPosition, 1);
                    particle.endPosition = matrix * glm::vec4(particle.endPosition, 1);
                    particle.beginScale *= transform.scale;
                    particle.endScale *= transform.scale;
                    particle.beginRotation += transform.rotation.z;
                    particle.endRotation += transform.rotation.z;
                }
            }
        });
    }

    TRI_UPDATE_CALLBACK("Particle"){
        TRI_PROFILE("particles");
        Ref<Mesh> quad = env->resources->get<Mesh>("quad");
        auto render = [&](const glm::mat4 &projection, glm::vec3 cameraPosition, glm::vec3 cameraRotation, const Ref<FrameBuffer> &frameBuffer){
            {
                TRI_PROFILE("particles/begin");
                RenderContext::setBlend(true);
                env->renderer->begin(projection, cameraPosition, frameBuffer);
            }
            {
                TRI_PROFILE("particles/submit");
                env->scene->view<Particle>().each([&](EntityId id, Particle &particle){
                    if(env->time->time > particle.spawnTime + particle.lifeTime){
                        env->scene->destroy(id);
                    }

                    float timeLine = (env->time->time - particle.spawnTime) / particle.lifeTime;

                    Color color = lerp(particle.beginColor.vec(), particle.endColor.vec(), timeLine);
                    glm::vec3 position = lerp(particle.beginPosition, particle.endPosition, timeLine);
                    float rotation = lerp(particle.beginRotation, particle.endRotation, timeLine);
                    glm::vec3 scale = lerp(particle.beginScale, particle.endScale, timeLine);

                    float alphaFactor = 1;
                    alphaFactor = std::min(alphaFactor, timeLine * particle.lifeTime / particle.fadeInTime);
                    alphaFactor = std::min(alphaFactor, (1.0f - timeLine) * particle.lifeTime / particle.fadeOutTime);
                    color.a *= alphaFactor;

                    glm::vec3 forward = glm::normalize(position - cameraPosition);
                    glm::vec3 rot = {-std::atan2(forward.y, forward.z), std::asin(forward.x), rotation};
                    env->renderer->submit({position, scale, rot, color}, particle.texture.get());
                });
            }
            {
                TRI_PROFILE("particles/end");
                env->renderer->end();
                RenderContext::setBlend(false);
            }
        };

        env->scene->view<PerspectiveCamera, Transform>().each([&](PerspectiveCamera &camera, Transform &transform){
            camera.forward = transform.getMatrix() * glm::vec4(0, 0, 1, 0);
            camera.up = transform.getMatrix() * glm::vec4(0, 1, 0, 0);
            camera.right = glm::cross(camera.forward, camera.up);
            if(camera.target.get() == nullptr){
                camera.aspectRatio = env->window->getAspectRatio();
            }
            render(camera.getProjection() * glm::inverse(transform.getMatrix()), transform.position, transform.rotation, camera.target);
            camera.output = camera.target;
        });
        env->scene->view<OrthographicCamera, Transform>().each([&](OrthographicCamera &camera, Transform &transform){
            camera.right = transform.getMatrix() * glm::vec4(1, 0, 0, 0);
            camera.up = transform.getMatrix() * glm::vec4(0, 1, 0, 0);
            if(camera.target.get() == nullptr) {
                camera.aspectRatio = env->window->getAspectRatio();
            }
            render(camera.getProjection() * glm::inverse(transform.getMatrix()), transform.position, transform.rotation, camera.target);
            camera.output = camera.target;
        });

    }

}

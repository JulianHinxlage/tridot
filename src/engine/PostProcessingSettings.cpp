//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "PostProcessingSettings.h"
#include "render/renderer/RenderSettings.h"
#include "core/core.h"
#include "entity/World.h"
#include "engine/Time.h"

namespace tri {

    TRI_COMPONENT(PostProcessingSettings);
    TRI_PROPERTIES8(PostProcessingSettings, hueShift, saturation, temperature, contrast, brightness, gamma, gain, fadeTime);
    TRI_PROPERTY_RANGE(PostProcessingSettings, hueShift, 0, 1);
    TRI_PROPERTY_RANGE(PostProcessingSettings, saturation, 0, 2);
    TRI_PROPERTY_RANGE(PostProcessingSettings, temperature, 0, 4);
    TRI_PROPERTY_RANGE(PostProcessingSettings, contrast, 0, 2);
    TRI_PROPERTY_RANGE(PostProcessingSettings, brightness, 0, 2);
    TRI_PROPERTY_RANGE(PostProcessingSettings, gamma, 0, 4);
    TRI_FUNCTION(PostProcessingSettings, enable);
    TRI_FUNCTION(PostProcessingSettings, disable);

    void PostProcessingSettings::enable() {
        if (env->renderSettings) {
            if (!active) {
                if (fadeStartTime < 0 || env->time->time - fadeStartTime > fadeTime) {
                    lastHueShift = env->renderSettings->hueShift;
                    lastSaturation = env->renderSettings->saturation;
                    lastTemperature = env->renderSettings->temperature;
                    lastContrast = env->renderSettings->contrast;
                    lastBrightness = env->renderSettings->brightness;
                    lastGamma = env->renderSettings->gamma;
                    lastGain = env->renderSettings->gain;
                }
                if (fadeStartTime < 0) {
                    fadeStartTime = env->time->time;
                }
                else {
                    fadeStartTime = env->time->time - glm::max(0.0f, (fadeTime - (env->time->time - fadeStartTime)));
                }
                env->world->each<PostProcessingSettings>([&](EntityId id, PostProcessingSettings& settings) {
                    if (&settings != this) {
                        if (settings.active) {
                            previousSettingsEntity = id;
                        }
                        settings.active = false;
                        settings.fadeStartTime = -1;
                    }
                });
                active = true;
            }
        }
    }

    void PostProcessingSettings::disable() {
        if (env->renderSettings) {
            if (active) {
                if (fadeStartTime < 0) {
                    fadeStartTime = env->time->time;
                }
                else {
                    fadeStartTime = env->time->time - glm::max(0.0f, (fadeTime - (env->time->time - fadeStartTime)));
                }
                active = false;
                if (previousSettingsEntity != -1) {
                    if (auto* settings = env->world->getComponent<PostProcessingSettings>(previousSettingsEntity)) {
                        EntityId prev = settings->previousSettingsEntity;
                        settings->enable();
                        settings->previousSettingsEntity = prev;
                    }
                    previousSettingsEntity = -1;
                }
            }
        }
    }

    class PostProcessingSettingsSystem : public System {
    public:
        void tick() override {
            env->world->each<PostProcessingSettings>([](PostProcessingSettings& settings) {

                float factor = 0;
                bool apply = false;
                if (settings.active) {
                    if (settings.fadeStartTime >= 0) {
                        if (settings.fadeTime == 0) {
                            factor = 1;
                            apply = true;
                        }
                        else {
                            factor = (env->time->time - settings.fadeStartTime) / settings.fadeTime;
                            factor = glm::max(0.0f, glm::min(1.0f, factor));
                            apply = true;
                        }
                    }
                }
                else {
                    if (settings.fadeStartTime >= 0) {
                        if (settings.fadeTime == 0) {
                            factor = 0;
                            apply = true;
                            settings.fadeStartTime = -1;
                        }
                        else {
                            factor = (env->time->time - settings.fadeStartTime) / settings.fadeTime;
                            apply = true;
                            if (factor >= 1.0) {
                                settings.fadeStartTime = -1;
                            }
                            factor = 1.0f - glm::max(0.0f, glm::min(1.0f, factor));
                        }
                    }
                }

                if (apply) {
                    if (env->renderSettings) {
                        env->renderSettings->hueShift = glm::mix(settings.lastHueShift, settings.hueShift, factor);
                        env->renderSettings->saturation = glm::mix(settings.lastSaturation, settings.saturation, factor);
                        env->renderSettings->temperature = glm::mix(settings.lastTemperature, settings.temperature, factor);
                        env->renderSettings->contrast = glm::mix(settings.lastContrast, settings.contrast, factor);
                        env->renderSettings->brightness = glm::mix(settings.lastBrightness, settings.brightness, factor);
                        env->renderSettings->gamma = glm::mix(settings.lastGamma, settings.gamma, factor);
                        env->renderSettings->gain = glm::mix(settings.lastGain.vec(), settings.gain.vec(), factor);
                    }
                }

            });
        }
    };
    TRI_SYSTEM(PostProcessingSettingsSystem);

}

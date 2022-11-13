//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "render/objects/Material.h"
#include "render/objects/Mesh.h"

namespace tri {

    class PostProcessingSettings {
    public:
        float hueShift = 0.0f;
        float saturation = 1.0f;
        float temperature = 1.0f;
        float contrast = 1.0f;
        float brightness = 1.0f;
        float gamma = 1.0f;
        Color gain = color::black;
        float fadeTime = 1.0f;

        bool active = false;
        float fadeStartTime = -1;
        EntityId previousSettingsEntity = -1;
        float lastHueShift = 0.0f;
        float lastSaturation = 1.0f;
        float lastTemperature = 1.0f;
        float lastContrast = 1.0f;
        float lastBrightness = 1.0f;
        float lastGamma = 1.0f;
        Color lastGain = color::black;

        void enable();
        void disable();
    };

}

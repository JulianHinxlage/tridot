//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "tridot/util/Clock.h"
#include <vector>

namespace tridot {

    class Time {
    public:
        //info
        float deltaTime;
        float frameTime;
        float time;
        float frameCounter;

        //options
        float deltaTimeFactor;
        float maxDeltaTime;
        bool pause;

        //stats
        float framesPerSecond;
        float avgFrameTime;
        float minFrameTime;
        float maxFrameTime;

        Time();
        void init();
        void update();

        //utility
        int frameTicks(float interval, float offset = 0);
        int deltaTicks(float interval, float offset = 0);

    private:
        Clock clock;
        std::vector<float> frameTimes;
        float frameTimeAccumulator;
        float deltaTimeAccumulator;
        float lastFrameTimeAccumulator;
        float lastDeltaTimeAccumulator;
    };

}


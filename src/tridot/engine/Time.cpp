//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Time.h"
#include <cmath>

namespace tridot {

    Time::Time() {
        //info
        deltaTime = 0;
        frameTime = 0;
        time = 0;
        frameCounter = 0;

        //options
        deltaTimeFactor = 1;
        maxDeltaTime = 0.2;
        pause = false;

        //stats
        framesPerSecond = 0;
        avgFrameTime = 0;
        minFrameTime = 0;
        maxFrameTime = 0;

        //accumulators
        frameTimeAccumulator = -1;
        deltaTimeAccumulator = 0;
        lastFrameTimeAccumulator = 0;
        lastDeltaTimeAccumulator = 0;
    }

    void Time::init() {
        clock.reset();
        frameTimeAccumulator = 0;
        deltaTimeAccumulator = 0;
        lastFrameTimeAccumulator = 0;
        lastDeltaTimeAccumulator = 0;
        frameCounter = 0;
    }

    void Time::update() {
        if(frameTimeAccumulator < 0){
            init();
            return;
        }

        //frame/delta time
        frameTime = (float)clock.round();
        deltaTime = pause ? 0.0f : std::min(maxDeltaTime, frameTime * deltaTimeFactor);
        frameCounter++;

        //accumulation
        lastDeltaTimeAccumulator = deltaTimeAccumulator;
        lastFrameTimeAccumulator = frameTimeAccumulator;
        deltaTimeAccumulator += deltaTime;
        frameTimeAccumulator += frameTime;
        time = frameTimeAccumulator;

        //stats
        frameTimes.push_back(frameTime);
        minFrameTime = INFINITY;
        maxFrameTime = -INFINITY;
        float sum = 0;
        for(int i = frameTimes.size() - 1; i >= 0; i--){
            sum += frameTimes[i];
            minFrameTime = std::min(minFrameTime, frameTimes[i]);
            maxFrameTime = std::max(maxFrameTime, frameTimes[i]);
            if(sum >= 1.0f){
                frameTimes.erase(frameTimes.begin(), frameTimes.begin() + i);
                break;
            }
        }
        avgFrameTime = sum / frameTimes.size();
        framesPerSecond = 1.0f / avgFrameTime;
    }

    int Time::frameTicks(float interval, float offset) {
        int ticks1 = (int)((lastFrameTimeAccumulator + offset) / interval);
        int ticks2 = (int)((frameTimeAccumulator + offset) / interval);
        return ticks2 - ticks1;
    }

    int Time::deltaTicks(float interval, float offset) {
        int ticks1 = (int)((lastDeltaTimeAccumulator + offset) / interval);
        int ticks2 = (int)((deltaTimeAccumulator + offset) / interval);
        return ticks2 - ticks1;
    }

}
//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Time.h"
#include "RuntimeMode.h"
#include <cmath>
#include <algorithm>

namespace tri {

    TRI_SYSTEM_INSTANCE(Time, env->time);

    void preciseSleep(double seconds) {
        using namespace std;
        using namespace std::chrono;

        static double estimate = 5e-3;
        static double mean = 5e-3;
        static double m2 = 0;
        static int64_t count = 1;

        while (seconds > estimate) {
            auto start = high_resolution_clock::now();
            this_thread::sleep_for(milliseconds(1));
            auto end = high_resolution_clock::now();

            double observed = (end - start).count() / 1e9;
            seconds -= observed;

            ++count;
            double delta = observed - mean;
            mean += delta / count;
            m2 += delta * (observed - mean);
            double stddev = sqrt(m2 / (count - 1));
            estimate = mean + stddev;
        }

        // spin lock
        auto start = high_resolution_clock::now();
        while ((high_resolution_clock::now() - start).count() / 1e9 < seconds);
    }

    Time::Time() {
        //info
        deltaTime = 0;
        frameTime = 0;
        time = 0;
        inGameTime = 0;
        frameCounter = 0;

        //options
        deltaTimeFactor = 1;
        maxDeltaTime = 0.2;
        pause = false;
        frameRateLimit = -1;

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
        env->console->addCVar("frameRateLimit", &frameRateLimit);

        class Replay {
        public:
            std::string command;
            float time = 0;
        };
        static std::vector<Replay> replays;
        env->console->addCommand("replay", [&](auto& args) {
            bool usage = false;
            if (args.size() > 0) {
                if (args[0] == "clear") {
                    replays.clear();
                }
                else {
                    float time = 0;
                    try {
                        time = std::stof(args[0]);

                        replays.push_back({});
                        replays.back().time = time;
                        auto args2 = args;
                        args2.erase(args2.begin());
                        replays.back().command = StrUtil::join(args2, " ");
                    }
                    catch (...) {
                        usage = true;
                    }
                }

            }
            else {
                usage = true;
            }

            if (usage) {
                env->console->info("usage: replay clear");
                env->console->info("usage: replay <time> <command>");
            }
        });
        env->eventManager->postTick.addListener([&]() {
            for (auto& replay : replays) {
                if (env->time->frameTicks(replay.time)) {
                    env->console->executeCommand(replay.command);
                }
            }
        });
    }
    
    void Time::startup() {
        clock.reset();
        frameTimeAccumulator = 0;
        deltaTimeAccumulator = 0;
        lastFrameTimeAccumulator = 0;
        lastDeltaTimeAccumulator = 0;
        frameCounter = 0;
    }

    void Time::tick() {
        //frame/delta time
        if (frameRateLimit > 0) {            
            frameTime = (float)clock.elapsed();
            float sleepTime = (1.0f / frameRateLimit) - frameTime;
            if (sleepTime > 0) {
                preciseSleep(sleepTime);
            }
            frameTime = (float)clock.round();
        }
        else {
            frameTime = (float)clock.round();
        }

        if (env->runtimeMode->getMode() == RuntimeMode::PLAY) {
            deltaTime = pause ? 0.0f : std::min(maxDeltaTime, frameTime * deltaTimeFactor);
        }
        else {
            deltaTime = 0;
        }

        frameCounter++;

        //accumulation
        lastDeltaTimeAccumulator = deltaTimeAccumulator;
        lastFrameTimeAccumulator = frameTimeAccumulator;
        deltaTimeAccumulator += deltaTime;
        frameTimeAccumulator += frameTime;
        time = frameTimeAccumulator;
        inGameTime = deltaTimeAccumulator;

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
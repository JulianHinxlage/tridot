//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Clock.h"

namespace tri {

    uint64_t nowNano(){
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }

    Clock::Clock() {
        startTime = 0;
        reset();
    }

    void Clock::reset() {
        startTime = nowNano();
    }

    double Clock::elapsed() {
        uint64_t time = nowNano();
        uint64_t diff = time - startTime;
        return (double)diff / 1000.0 / 1000.0 / 1000.0;
    }

    double Clock::round() {
        uint64_t time = nowNano();
        uint64_t diff = time - startTime;
        startTime = time;
        return (double)diff / 1000.0 / 1000.0 / 1000.0;
    }

    double Clock::now() {
        return (double)nowNano() / 1000.0 / 1000.0 / 1000.0;
    }

    void Clock::sleep(double seconds){
        std::this_thread::sleep_for(std::chrono::nanoseconds((long long)(seconds * 1000.0 * 1000.0 * 1000.0)));
    }

}
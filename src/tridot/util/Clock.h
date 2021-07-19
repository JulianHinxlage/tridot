//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <cstdint>

namespace tridot {

    class Clock {
    public:
        Clock();
        void reset();
        double elapsed();
        double round();
        static double now();
    private:
        uint64_t startTime;
    };

}


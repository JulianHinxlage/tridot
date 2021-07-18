//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_CLOCK_H
#define TRIDOT_CLOCK_H

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

#endif //TRIDOT_CLOCK_H

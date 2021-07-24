//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

namespace tridot {

    class Imgui {
    public:
        void init();
        void begin();
        void end();

    private:
        bool inFrame;
    };

}

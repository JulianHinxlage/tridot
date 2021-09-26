//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

namespace tri {

    enum RuntimeMode {
        EDIT,
        RUNTIME,
        PAUSED,
    };

    class EditorWindow {
    public:
        std::string name;
        bool isOpen = false;
        bool isDebugWindow = false;
        bool isWindow = true;
        std::string profileName;

        virtual void startup() {}
        virtual void update() {}
        virtual void shutdown() {}
    };

}


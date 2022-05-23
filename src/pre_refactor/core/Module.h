//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

namespace tri {

    class Module {
    public:
        Module() {}
        virtual ~Module() {}
        virtual void startup() {};
        virtual void update() {};
        virtual void shutdown() {};
    };

}

#define TRI_REGISTER_MODULE(ModuleClass) extern "C" tri::Module *triCreateModuleInstance(){return new ModuleClass();}

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "ComponentRegister.h"

namespace tridot {

    TypeMap ComponentRegister::componentMap;
    std::vector<std::shared_ptr<Pool>> ComponentRegister::componentPools;

}

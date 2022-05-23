#include "Reflection.h"
//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "Environment.h"
#include "Reflection.h"
#include "SignalManager.h"

namespace tri {

    void Reflection::unregisterType(int typeId) {
        if (typeId >= 0 && typeId < descriptors.size()) {
            env->signals->typeUnregister.invoke(typeId);
            descriptorsByName.erase(descriptors[typeId]->name);
            descriptors[typeId] = nullptr;
        }
    }

    void Reflection::update() {
        for (auto desc : descriptors) {
            if (desc && !desc->initFlag) {
                desc->initFlag = true;
                env->signals->typeRegister.invoke(desc->typeId);
            }
        }

    }

}
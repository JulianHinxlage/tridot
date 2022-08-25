//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "Blob.h"

namespace tri {

    class Guid : public Blob<128> {
    public:
        Guid();
        std::string toString();
        bool fromString(const std::string& str);
    };

}


//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "core/config.h"
#include <string>
#include <vector>

namespace tri {

    class TRI_API StrUtil{
    public:
        //0 = no, 1 = yes, 2 = yes at beginning, 3 = yes at end
        static int isSubstring(const std::string &string1, const std::string &string2);
        static int match(const std::string &string1, const std::string &string2, bool reverse = false);
        static std::vector<std::string> split(const std::string& string, const std::string& delimiter, bool includeEmpty = true);
        static std::vector<std::string> split(const std::vector<std::string> &strings, const std::string &delimiter, bool includeEmpty = true);
        static std::string join(const std::vector<std::string> &strings, const std::string &delimiter);
        static std::string replace(const std::string &string, const std::string &search, const std::string &replacement);
        static std::string readFile(const std::string &file);
    };

}


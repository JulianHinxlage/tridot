//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "StrUtil.h"
#include <algorithm>

namespace tri {

    int StrUtil::isSubstring(const std::string &string1, const std::string &string2){
        auto pos = string1.find(string2);
        if(pos == std::string::npos){
            return 0;
        }else if(pos == 0){
            return 2;
        }else if(pos == abs((int)string1.size() - (int)string2.size())){
            return 3;
        }else{
            return 1;
        }
    }

    int StrUtil::match(const std::string &string1, const std::string &string2, bool reverse) {
        int match = 0;
        int size = (int)std::min(string1.size(), string2.size());
        if(reverse){
            for(int i = size-1; i >= 0; i--){
                if(string1[i] == string2[i]){
                    match++;
                }else{
                    break;
                }
            }
        }else{
            for(int i = 0; i < size; i++){
                if(string1[i] == string2[i]){
                    match++;
                }else{
                    break;
                }
            }
        }
        return match;
    }

    std::vector<std::string> StrUtil::split(const std::string &string, const std::string &delimiter, bool includeEmpty) {
        std::vector<std::string> parts;
        std::string token;
        int delimiterIndex = 0;
        for(char c : string){
            if((int)delimiter.size() == 0){
                parts.push_back({c, 1});
            }else if(c == delimiter[delimiterIndex]){
                delimiterIndex++;
                if(delimiterIndex == delimiter.size()){
                    if(includeEmpty || (int)token.size() != 0){
                        parts.push_back(token);
                    }
                    token.clear();
                    delimiterIndex = 0;
                }
            }else{
                token += delimiter.substr(0, delimiterIndex);
                token.push_back(c);
                delimiterIndex = 0;
            }
        }
        token += delimiter.substr(0, delimiterIndex);
        if(includeEmpty || (int)token.size() != 0){
            parts.push_back(token);
        }
        return parts;
    }

    std::vector<std::string> StrUtil::split(const std::vector<std::string>& strings, const std::string& delimiter, bool includeEmpty){
        std::vector<std::string> parts;
        for (auto& s : strings) {
            for (auto& i : split(s, delimiter, includeEmpty)) {
                parts.push_back(i);
            }
        }
        return parts;
    }

    std::string StrUtil::join(const std::vector<std::string> &strings, const std::string &delimiter) {
        std::string result;
        for(int i = 0; i < strings.size(); i++){
            result += strings[i];
            if(i != strings.size() - 1){
                result += delimiter;
            }
        }
        return result;
    }

    std::string StrUtil::replace(const std::string &string, const std::string & search, const std::string &replacement) {
        return join(split(string, search), replacement);
    }

    std::string StrUtil::readFile(const std::string& file){
        std::ifstream stream(file);
        if (stream.is_open()) {
            return std::string((std::istreambuf_iterator<char>(stream)), (std::istreambuf_iterator<char>()));
        }
        else {
            return "";
        }
    }

    std::string StrUtil::toLower(const std::string& str) {
        std::string result;
        for (char c : str) {
            result.push_back(std::tolower(c));
        }
        return result;
    }

    std::string StrUtil::toUpper(const std::string& str) {
        std::string result;
        for (char c : str) {
            result.push_back(std::toupper(c));
        }
        return result;
    }

}

//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <vector>
#include <string>

namespace tri {

    class Packet {
    public:
        std::vector<uint8_t> buffer;
        int readIndex;
        int writeIndex;

        Packet();
    
        //for writing
        void reserve(int bytes);
        void resize(int bytes, uint8_t value);
        void add(const void* ptr, int bytes);
        void addStr(const std::string& str);
        std::string toString();

        template<typename T>
        void add(const T& t) {
            add(&t, sizeof(t));
        }

        //for reading
        void get(void* ptr, int bytes);
        std::string getStr();
        void skip(int bytes);
        void unskip(int bytes);
        std::string getRemaining();

        template<typename T>
        T get() {
            T t;
            get(&t, sizeof(t));
            return t;
        }

        //for writing and reading
        uint8_t* data();
        int size();
    };

}

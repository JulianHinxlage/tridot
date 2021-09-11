//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "util/Clock.h"

namespace tri {

    class Profiler : public System {
    public:
        void begin(const char *name);
        void end(const char *name);
        virtual void update();
        void updateNodes();

        class Node{
        public:
            std::string name;
            float time;
            float count;
            std::vector<Node> nodes;
            Node *get(const std::string &name);
            void clear();
            float update();
        };
        Node node;
        float interval = 1.0f;
    private:
        class Entry{
        public:
            Clock clock;
            bool inProgress;
            std::vector<float> times;
            std::vector<float> counts;
            std::vector<float> sums;
            float sum();
            float avgSum();
            float avgCount();
        };
        std::unordered_map<const char *, Entry> entries;
    };

    namespace impl {
        class ProfileScope {
        public:
            const char *name;
            ProfileScope(const char *name);
            ~ProfileScope();
        };
    }
}

#ifdef TRI_DEBUG
#define TRI_PROFILE(name) tri::impl::ProfileScope profileScope(name);
#else
#define TRI_PROFILE(name)
#endif
#define TRI_PROFILE_FUNC() TRI_PROFILE(__FUNCTION__)


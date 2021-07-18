//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_PROFILER_H
#define TRIDOT_PROFILER_H

#include "tridot/util/Clock.h"
#include <vector>
#include <unordered_map>

namespace tridot {

    class Profiler {
    public:
        void begin(const char *name);
        void end(const char *name);
        void update();

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
#define TRI_PROFILE(name) tridot::impl::ProfileScope profileScope(name);
#elif
#define TRI_PROFILE(name)
#endif

#endif //TRIDOT_PROFILER_H

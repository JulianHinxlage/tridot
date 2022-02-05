//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "System.h"
#include "util/Clock.h"
#include "tracy/Tracy.hpp"

namespace tri {

    class Profiler : public System {
    public:
        Profiler();

        void begin(const std::string &name);
        void end();

        void beginPhase(const std::string &name);
        void endPhase();

        void nextFrame();
        int getCurrentFrame();

        class Node{
        public:
            std::string name;
            std::shared_ptr<Node> parent;
            std::vector<std::shared_ptr<Node>> children;
            std::unordered_map<int, std::vector<float>> frames;
            Clock clock;
        };
        const std::vector<std::shared_ptr<Node>> &getPhaseNodes(){
            return phases;
        }

    private:
        std::vector<std::shared_ptr<Node>> phases;
        thread_local static std::shared_ptr<Node> currentNode;
        thread_local static std::vector<std::shared_ptr<Node>> phaseNodeStack;
        int currentFrame;
        std::mutex mutex;
    };

    namespace impl {
        class ProfileScope {
        public:
            ProfileScope(const std::string &name);
            ~ProfileScope();
        };
        class ProfilePhase {
        public:
            ProfilePhase(const std::string &name);
            ~ProfilePhase();
        };
    }

}

#ifdef TRI_DEBUG
    #if TRI_ENABLE_OWN_PROFILER
        #define TRI_PROFILE(name) tri::impl::ProfileScope profileScope(name); ZoneScopedN(name);
        #define TRI_PROFILE_NAME(name, size) tri::impl::ProfileScope profileScope(name); ZoneScoped; ZoneName(name, size);
        #define TRI_PROFILE_PHASE(name) tri::impl::ProfilePhase profilePhase(name);
        #define TRI_PROFILE_FUNC() TRI_PROFILE(__FUNCTION__) ZoneScoped;
        #define TRI_PROFILE_THREAD(name) tracy::SetThreadName(name);
        #define TRI_PROFILE_INFO(text, size) ZoneText(text, size)
        #define TRI_PROFILE_FRAME env->profiler->nextFrame(); FrameMark;
    #else
        #define TRI_PROFILE(name) ZoneScopedN(name);
        #define TRI_PROFILE_NAME(name, size) ZoneScoped; ZoneName(name, size);
        #define TRI_PROFILE_PHASE(name) 
        #define TRI_PROFILE_FUNC() ZoneScoped;
        #define TRI_PROFILE_THREAD(name) tracy::SetThreadName(name);
        #define TRI_PROFILE_INFO(text, size) ZoneText(text, size);
        #define TRI_PROFILE_FRAME FrameMark;
    #endif
#else
    #define TRI_PROFILE(name)
    #define TRI_PROFILE_NAME(name, size)
    #define TRI_PROFILE_PHASE(name)
    #define TRI_PROFILE_FUNC()
    #define TRI_PROFILE_THREAD(name)
    #define TRI_PROFILE_INFO(text, size)
    #define TRI_PROFILE_FRAME
#endif


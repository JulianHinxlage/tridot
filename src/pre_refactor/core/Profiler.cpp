//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Profiler.h"
#include "util/StrUtil.h"
#include "Environment.h"

namespace tri {

    thread_local std::shared_ptr<Profiler::Node> Profiler::currentNode;
    thread_local std::vector<std::shared_ptr<Profiler::Node>> Profiler::phaseNodeStack;

    Profiler::Profiler() {
        currentFrame = 0;
        currentNode = nullptr;
    }

    void Profiler::begin(const std::string &name) {
        if(currentNode){
            for(auto &node : currentNode->children){
                if(node && node->name == name){
                    node->clock.reset();
                    currentNode = node;
                    return;
                }
            }
            mutex.lock();
            std::shared_ptr<Node> node = std::make_shared<Node>();
            node->name = name;
            node->parent = currentNode;
            node->clock.reset();
            currentNode->children.push_back(node);
            currentNode = node;
            mutex.unlock();
        }
    }

    void Profiler::end() {
        if(currentNode){
            double elapsed = currentNode->clock.elapsed();
            currentNode->frames[currentFrame].push_back(elapsed);
            currentNode = currentNode->parent;
        }
    }

    void Profiler::beginPhase(const std::string &name) {
        if(currentNode != nullptr){
            phaseNodeStack.push_back(currentNode);
        }
        for(auto &node : phases){
            if(node && node->name == name){
                node->clock.reset();
                currentNode = node;
                return;
            }
        }
        mutex.lock();
        std::shared_ptr<Node> node = std::make_shared<Node>();
        node->name = name;
        node->parent = nullptr;
        node->clock.reset();
        phases.push_back(node);
        currentNode = node;
        mutex.unlock();
    }

    void Profiler::endPhase() {
        if(currentNode){
            double elapsed = currentNode->clock.elapsed();
            currentNode->frames[currentFrame].push_back(elapsed);
        }
        if(!phaseNodeStack.empty()){
            currentNode = phaseNodeStack.back();
            phaseNodeStack.pop_back();
        }else{
            currentNode = nullptr;
        }
    }

    void Profiler::nextFrame() {
        currentFrame++;
    }

    int Profiler::getCurrentFrame() {
        return currentFrame;
    }

    impl::ProfileScope::ProfileScope(const std::string &name) {
        if(env->profiler){
            env->profiler->begin(name);
        }
    }

    impl::ProfileScope::~ProfileScope() {
        if(env->profiler) {
            env->profiler->end();
        }
    }

    impl::ProfilePhase::ProfilePhase(const std::string &name) {
        if(env->profiler){
            env->profiler->beginPhase(name);
        }
    }

    impl::ProfilePhase::~ProfilePhase() {
        if(env->profiler) {
            env->profiler->endPhase();
        }
    }
}

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Profiler.h"
#include "tridot/util/StrUtil.h"
#include "tridot/core/Environment.h"
#include "tridot/engine/Time.h"

namespace tridot {

    void Profiler::begin(const char *name) {
        auto &entry = entries[name];
        if (!entry.inProgress) {
            entry.inProgress = true;
            entry.clock.reset();
        }
    }

    void Profiler::end(const char *name) {
        auto &entry = entries[name];
        if(entry.inProgress){
            entry.inProgress = false;
            entry.times.push_back(entry.clock.elapsed());
        }
    }

    void Profiler::update() {
        for(auto &entry : entries){
            entry.second.sums.push_back(entry.second.sum());
            entry.second.counts.push_back(entry.second.times.size());
            entry.second.times.clear();
        }
        if(env->time->frameTicks(interval)) {
            node.clear();
            for(auto &entry : entries){
                Node *node = &this->node;
                if(std::string(entry.first) != "total"){
                    auto parts = StrUtil::split(entry.first, "/");
                    for(auto &name : parts){
                        node = node->get(name);
                    }
                }else{
                    node->name = entry.first;
                }
                node->time += entry.second.avgSum();
                node->count += entry.second.avgCount();
                entry.second.sums.clear();
                entry.second.counts.clear();
            }
            node.update();
        }
    }

    Profiler::Node *Profiler::Node::get(const std::string &name){
        for(auto &n : nodes){
            if(n.name == name){
                return &n;
            }
        }
        for(int i = 0; i < nodes.size(); i++){
            if(nodes[i].name >= name){
                nodes.insert(nodes.begin() + i, Node());
                nodes[i].name = name;
                return &nodes[i];
            }
        }
        nodes.emplace_back();
        nodes.back().name = name;
        return &nodes.back();
    }

    void Profiler::Node::clear(){
        time = 0;
        count = 0;
        for(auto &n : nodes){
            n.clear();
        }
    }

    float Profiler::Node::update() {
        float sum = 0;
        for(auto &n : nodes){
            sum += n.update();
        }
        if(count == 0){
            time = sum;
            if(sum != 0){
                count = 1;
            }
        }
        return time;
    }

    float Profiler::Entry::sum() {
        float value = 0;
        for(float time : times){
            value += time;
        }
        return value;
    }

    float Profiler::Entry::avgSum() {
        float value = 0;
        for(float time : sums){
            value += time;
        }
        if(sums.size() == 0){
            return 0;
        }else{
            return value / sums.size();
        }
    }

    float Profiler::Entry::avgCount() {
        float value = 0;
        for(float time : counts){
            value += time;
        }
        if(counts.size() == 0){
            return 0;
        }else{
            return value / counts.size();
        }
    }

    namespace impl {

        ProfileScope::ProfileScope(const char *name) {
            if(env->profiler){
                env->profiler->begin(name);
            }
            this->name = name;
        }

        ProfileScope::~ProfileScope() {
            if(env->profiler) {
                env->profiler->end(name);
            }
        }

    }

}

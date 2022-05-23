//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "Profiler.h"
#include "Reflection.h"
#include "Environment.h"
#include "EventManager.h"

namespace tri {

	TRI_SYSTEM_INSTANCE(Profiler, env->profiler);

	thread_local Profiler::Node* Profiler::currentNode = nullptr;

#if TRI_PROFILE_ENABLED

	static uint64_t nowNano() {
		return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	}

	static void check(Profiler::Node* node, const char* name) {
		node->nodes.erase(name);
		for (auto& n : node->nodes) {
			check(n.second.get(), name);
		}
	}

	void Profiler::init() {
		env->eventManager->onClassUnregister.addListener([this](int classId) {
			check(&root, Reflection::getDescriptor(classId)->name.c_str());
			currentNode = &root;
		});
		currentNode = &root;
		root.name = "frame";
	}
	
	void Profiler::shutdown() {
		root.nodes.clear();
		root.times.clear();
	}
	
	void Profiler::nextFrame() {
		end();
	}

	void Profiler::begin(const char* name) {
		if (!currentNode) {
			currentNode = &root;
		}
		auto& node = currentNode->nodes[name];
		if (!node) {
			node = std::make_shared<Node>();
			node->parent = currentNode;
			node->name = name;
		}
		node->beginTimeNano = nowNano();
		currentNode = node.get();
	}

	void Profiler::end() {
		uint64_t timeNano = nowNano();
		
		if (currentNode->beginTimeNano != 0) {
			double time = (double)(timeNano - currentNode->beginTimeNano) / 1000.0 / 1000.0 / 1000.0;
			double total = currentNode->time * currentNode->times.size();

			while (total > keepTimeSeconds) {
				total -= currentNode->times.front();
				currentNode->times.erase(currentNode->times.begin());
			}

			currentNode->times.push_back(time);
			currentNode->time = (total + time) / (currentNode->times.size());
			currentNode->beginTimeNano = 0;
		}
		if (currentNode->parent) {
			currentNode = currentNode->parent;
		}
		else {
			currentNode->beginTimeNano = timeNano;
		}
	}

#else
	void Profiler::init() {
		currentNode = &root;
		root.name = "frame";
	}
	void Profiler::nextFrame() {}
	void Profiler::begin(const char* name) {}
	void Profiler::end() {}
#endif
}




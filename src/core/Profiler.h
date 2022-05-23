//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "System.h"
#include "tracy/Tracy.hpp"

namespace tri {

	class Profiler : public System {
	public:
		virtual void init();
		virtual void shutdown();
		void nextFrame();
		void begin(const char *name);
		void end();

		class Node {
		public:
			double time = 0;
			double displayTime = 0;
			const char* name = nullptr;
			std::unordered_map<const char*, std::shared_ptr<Node>> nodes;
		private:
			friend class Profiler;
			uint64_t beginTimeNano = 0;
			std::vector<double> times;
			Node* parent;
		};
		Node* getRoot() { return &root; }

	private:
		Node root;
		double keepTimeSeconds = 1;
		static thread_local Node* currentNode;
	};

}

#if TRI_PROFILE_ENABLED
#define TRI_PROFILE(name) ZoneScopedN(name);
#define TRI_PROFILE_NAME(name, size) ZoneScoped; ZoneName(name, size);
#define TRI_PROFILE_FUNC() ZoneScoped;
#define TRI_PROFILE_THREAD(name) tracy::SetThreadName(name);
#define TRI_PROFILE_INFO(text, size) ZoneText(text, size);
#define TRI_PROFILE_FRAME FrameMark;
#else
#define TRI_PROFILE(name)
#define TRI_PROFILE_NAME(name, size)
#define TRI_PROFILE_FUNC()
#define TRI_PROFILE_THREAD(name)
#define TRI_PROFILE_INFO(text, size)
#define TRI_PROFILE_FRAME
#endif
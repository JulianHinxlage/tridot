//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

namespace tri {

	class MainLoop {
	public:
		static void init();
		static void parseArguments(int argc, char* args[], std::vector<std::string> defaultConfigFiles = {});
		static void startup();
		static void run();
		static void shutdown();
	};

}

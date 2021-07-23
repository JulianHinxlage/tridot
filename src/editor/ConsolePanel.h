//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "tridot/core/Environment.h"

namespace tridot {

	class ConsolePanel {
	public:
		void init();
		void update();
    private:
	    class Message{
	    public:
	        LogLevel level;
	        std::string message;
	    };
	    LogLevel logLevelFilter;
	    std::vector<Message> messages;
	    std::string inputBuffer;
	};

}


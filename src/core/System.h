//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

namespace tri {

	class System {
	public:
		virtual void init() {}
		virtual void startup() {}
		virtual void tick() {}
		virtual void shutdown() {}
	};

}

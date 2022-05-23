//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "System.h"

namespace tri {

	class FileWatcher : public System {
	public:
		double checkTimeInterval = 1;

		void startup() override;
		void addFile(const std::string& path, const std::function<void(const std::string &)>& onChange);

	private:
		class File {
		public:
			std::string path;
			std::function<void(const std::string&)> onChange;
			uint64_t time;
		};
		std::vector<File> files;
	};

}

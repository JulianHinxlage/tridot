//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "FileWatcher.h"
#include "Environment.h"
#include "ThreadManager.h"
#include "Profiler.h"

namespace tri {

	TRI_SYSTEM_INSTANCE(FileWatcher, env->fileWatcher);

	void FileWatcher::startup() {
		env->threadManager->addThread("File Watcher", [&]() {
			while (true) {
				std::this_thread::sleep_for(std::chrono::milliseconds((long long)(checkTimeInterval * 1000.0)));
				TRI_PROFILE("FileWatcher");
				for (auto& file : files) {
					uint64_t time = 0;
					try {
						if (std::filesystem::exists(file.absolutePath)) {
							time = std::filesystem::last_write_time(file.absolutePath).time_since_epoch().count();
						}
						if (time != file.time) {
							file.time = time;
							if (file.onChange) {
								file.onChange(file.path);
							}
						}
					}
					catch (...) {}
				}
			}
		});
	}

	void FileWatcher::addFile(const std::string& path, const std::function<void(const std::string&)>& onChange) {
		std::string absolutePath = std::filesystem::absolute(path).string();
		for (auto& file : files) {
			if (file.path == path || file.absolutePath == absolutePath) {
				return;
			}
		}
		File file;
		file.path = path;
		file.onChange = onChange;
		if (std::filesystem::exists(file.path)) {
			file.absolutePath = absolutePath;
			file.time = std::filesystem::last_write_time(file.absolutePath).time_since_epoch().count();
		}
		files.push_back(file);
	}

}



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
						time = std::filesystem::last_write_time(file.path).time_since_epoch().count();
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
		File file;
		file.path = path;
		file.path = std::filesystem::absolute(path).string();
		file.onChange = onChange;
		file.time = std::filesystem::last_write_time(file.path).time_since_epoch().count();
		files.push_back(file);
	}

}



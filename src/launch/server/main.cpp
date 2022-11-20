//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "core.h"
using namespace tri;

int main(int argc, char* argv[]) {
    MainLoop::init();
    env->config->loadConfigFileFirstFound({ "./server.cfg", "../server.cfg", "../../server.cfg" });
    MainLoop::startup();
    MainLoop::run();
    MainLoop::shutdown();
    return 0;
}

#if WIN32
int __stdcall WinMain(void* hInstance, void* hPrevInstance, char* lpCmdLine, int nCmdShow) {
    auto args = StrUtil::split(lpCmdLine, " ", false);
    std::vector<const char*> argv;
    argv.reserve(args.size());
    for (auto& a : args) {
        argv.push_back(a.c_str());
    }
    int argc = argv.size();
    return main(argc, (char**)argv.data());
}
#endif

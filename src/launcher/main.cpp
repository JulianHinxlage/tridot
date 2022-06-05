//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "core.h"
using namespace tri;

int main(int argc, char* argv[]) {
    MainLoop::init();
    env->config->loadConfigSearchList({ "config.txt", "../config.txt" });
    MainLoop::startup();
    MainLoop::run();
    MainLoop::shutdown();
    return 0;
}

#if WIN32
int __stdcall WinMain(void* hInstance, void* hPrevInstance, char* lpCmdLine, int nCmdShow) {
    return main(nCmdShow, (char**)lpCmdLine);
}
#endif

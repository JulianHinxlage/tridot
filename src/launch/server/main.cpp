//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "core.h"
using namespace tri;

#if WIN32
#include "windows.h"
BOOL onConsoleEvent(DWORD event) {
    switch (event) {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
        break;
    }
    return TRUE;
}
void setupHandler() {
    SetConsoleCtrlHandler(onConsoleEvent, TRUE);
}
#else
void setupHandler() {}
#endif

int main(int argc, char* argv[]) {
    setupHandler();
    MainLoop::init();
    MainLoop::parseArguments(argc, argv, { "./server.cfg", "../server.cfg", "../../server.cfg" });
    MainLoop::startup();
    MainLoop::run();
    MainLoop::shutdown();
    return 0;
}

#if WIN32
int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    auto args = StrUtil::split(lpCmdLine, " ", false);
    args.insert(args.begin(), std::filesystem::current_path().string() + "\\.exe");
    std::vector<const char*> argv;
    argv.reserve(args.size());
    for (auto& a : args) {
        argv.push_back(a.c_str());
    }
    int argc = argv.size();
    return main(argc, (char**)argv.data());
}
#endif




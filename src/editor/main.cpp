//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "engine/MainLoop.h"
#include "engine/AssetManager.h"

using namespace tri;

int main(int argc, char* argv[]) {
    env->assets->hotReloadEnabled = true;

    MainLoop loop;
    loop.startup({ "config.txt", "../config.txt", "../../config.txt" });
    loop.run();
    loop.shutdown();
}

#if TRI_WINDOWS
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) {
    return main(nCmdShow, (char**)lpCmdLine);
}
#endif

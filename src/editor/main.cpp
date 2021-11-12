//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "engine/MainLoop.h"
#include "engine/AssetManager.h"

using namespace tri;

int main(int argc, char* argv[]) {
    env->assets->hotReloadEnabled = true;

    MainLoop loop;
    loop.startup("config.txt", "../res/config.txt");
    loop.run();
    loop.shutdown();
}

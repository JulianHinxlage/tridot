//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "core.h"
#include "engine/AssetManager.h"
using namespace tri;

int main(int argc, char* argv[]) {
    MainLoop::init();
    env->config->loadConfigSearchList({ "configGame.txt", "../configGame.txt" });
    MainLoop::startup();

    //wait for all assets to be loaded before starting the scene
    while (env->assetManager->isLoadingInProcess()) {
        env->jobManager->tickJobs();
        if (!env->console->getCVarValue("running", false)) {
            break;
        }
    }

    MainLoop::run();
    MainLoop::shutdown();
    return 0;
}

#if WIN32
int __stdcall WinMain(void* hInstance, void* hPrevInstance, char* lpCmdLine, int nCmdShow) {
    return main(nCmdShow, (char**)lpCmdLine);
}
#endif

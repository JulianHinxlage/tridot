//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "core.h"
#include "window/Input.h"
#include "window/Window.h"
using namespace tri;

int main(int argc, char* argv[]) {
    MainLoop::init();
    env->config->loadConfigFileFirstFound({ "./configGame.txt", "../configGame.txt", "../../configGame.txt" });
    MainLoop::startup();

    //wait for all assets to be loaded before starting the scene
    env->console->executeCommand("waitForAllAssetsLoaded");

    env->eventManager->postTick.addListener([]() {
        if (env->input->down(Input::KEY_ESCAPE)) {
            env->window->close();
        }
    });

    MainLoop::run();
    MainLoop::shutdown();
    return 0;
}

#if WIN32
int __stdcall WinMain(void* hInstance, void* hPrevInstance, char* lpCmdLine, int nCmdShow) {
    return main(nCmdShow, (char**)lpCmdLine);
}
#endif

//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/core/config.h"
#include "tridot/render/Window.h"
#include "GL/gl.h"

using namespace tridot;

int main(int argc, char *argv[]){
    Log::info("Tridot version ", TRI_VERSION);

    Window window;
    window.init(800, 600, "Tridot " TRI_VERSION);
    while(window.isOpen()){
        window.update();
    }

    return 0;
}

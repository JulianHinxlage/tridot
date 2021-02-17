//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/core/config.h"
#include "tridot/render/Window.h"
#include "tridot/render/Shader.h"
#include "tridot/render/VertexArray.h"
#include "tridot/render/Texture.h"
#include "GL/gl.h"
#include "GLFW/glfw3.h"
#include <glm/gtc/matrix_transform.hpp>

using namespace tridot;

class Time{
public:
    float deltaTime;

    Time(){
        lastTime = 0;
        deltaTime = 0;
    }

    void update(){
        double t = glfwGetTime();
        deltaTime = t - lastTime;
        lastTime = t;
    }

private:
    double lastTime;
};
Time timer;


int main(int argc, char *argv[]){
    Log::options.logLevel = Log::TRACE;
    Log::info("Tridot version ", TRI_VERSION);
    Window window;
    window.init(800, 600, "Tridot " TRI_VERSION);

    Shader shader;
    shader.load("../res/shaders/shader.glsl");
    VertexArray vao;
    {
        Ref<Buffer> vbo(true);
        Ref<Buffer> vio(true);

        float vs[] = {
                -0.5, -0.5, 0, 0.5, 0.8, 0.5, 0.0, 0.0,
                -0.5, +0.5, 0, 0.5, 0.8, 0.5, 0.0, 1.0,
                +0.5, +0.5, 0, 0.5, 0.8, 0.5, 1.0, 1.0,
                +0.5, -0.5, 0, 0.5, 0.8, 0.5, 1.0, 0.0,
        };

        uint32_t is[] = {
                0, 1, 2,
                0, 2, 3,
        };

        vbo->init(vs, sizeof(vs), 6 * 4, false, false);
        vio->init(is, sizeof(is), 4, true, false);
        vao.addIndexBuffer(vio, UINT32);
        vao.addVertexBuffer(vbo, {{FLOAT, 3}, {FLOAT, 3}, {FLOAT, 2}});
    }

    Texture texture;
    texture.load("../res/textures/checkerboard.png");

    glm::vec2 pos(0, 0);
    glm::vec2 vel(0.1, 0.05);

    while(window.isOpen()){
        if(glfwGetKey((GLFWwindow*)window.getContext(), GLFW_KEY_ESCAPE) == GLFW_PRESS){
            window.close();
        }
        timer.update();

        pos += vel * timer.deltaTime;
        window.bind();
        shader.bind();
        shader.set("uTransform", glm::translate(glm::mat4(1), glm::vec3(pos, 0)));
        shader.set("uTexture", 0);
        texture.bind(0);

        vao.submit();
        window.update();
    }
    return 0;
}

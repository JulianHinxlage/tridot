//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/engine/Engine.h"
#include "tridot/components/RenderComponent.h"

using namespace tridot;

class Blink{
public:
    float speed = 0.5;
    Color color;
    bool flag;
    float value = 1;
};

TRI_REFLECT_TYPE(Blink)
TRI_REFLECT_MEMBER4(Blink, speed, color, flag, value)
TRI_COMPONENT(Blink)

extern "C" void update(){
    engine.view<Blink, RenderComponent>().each([](Blink &blink, RenderComponent &r){
        if(blink.flag){
            blink.value -= engine.time.deltaTime * blink.speed * 4;
            if(blink.value <= 0.0f){
                blink.flag = !blink.flag;
                blink.value = 0.0f;
            }
            r.color.r = blink.value * (255.0f - blink.color.r) + blink.color.r;
            r.color.g = blink.value * (255.0f - blink.color.g) + blink.color.g;
            r.color.b = blink.value * (255.0f - blink.color.b) + blink.color.b;
        }else{
            blink.value += engine.time.deltaTime * blink.speed * 4;
            if(blink.value >= 1.0f){
                blink.flag = !blink.flag;
                blink.value = 1.0f;
            }
            r.color.r = blink.value * (255.0f - blink.color.r) + blink.color.r;
            r.color.g = blink.value * (255.0f - blink.color.g) + blink.color.g;
            r.color.b = blink.value * (255.0f - blink.color.b) + blink.color.b;
        }
    });
}

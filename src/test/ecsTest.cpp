//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "tridot/ecs/Registry.h"
#include "tridot/core/Log.h"

using namespace ecs;
using namespace tridot;

class A{
public:
    int a;

    A(int a = 0) : a(a){}
};

class B{
public:
    int b;

    B(int b = 0) : b(b){}
};

int main(){
    Registry reg;

    reg.onCreate().add([](EntityId id){
        Log::info("created id = ", id);
    });
    reg.onDestroy().add( [](EntityId id){
        Log::info("destroyed id = ", id);
    });
    reg.onAdd<A>().add([](EntityId id){
        Log::info("added A id = ", id);
    });
    reg.onRemove<A>().add([](EntityId id){
        Log::info("removed A id = ", id);
    });

    reg.create(A(0));
    reg.create(B(1));
    reg.create(A(2), B(2));

    Log::info("");
    reg.view<>().each([](EntityId id){
        Log::info(id);
    });

    Log::info("");
    reg.view<A>().each([](EntityId id, A &a){
        Log::info(id, " ", a.a);
    });

    Log::info("");
    reg.view<B>().each([](EntityId id, B &b){
        Log::info(id, " ", b.b);
    });

    Log::info("");
    reg.view<A, B>().each([](EntityId id, A &a, B &b){
        Log::info(id, " ", a.a, " ", b.b);
    });

    Log::info("");
    reg.view<A>().excluded<B>().each([](EntityId id, A &a){
        Log::info(id, " ", a.a);
    });

    Log::info("");
    reg.view<B>().excluded<A>().each([](EntityId id, B &b){
        Log::info(id, " ", b.b);
    });

    Log::info("");
    reg.destroy(2);
    return 0;
}
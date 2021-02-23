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

    reg.create(A(0), B(0));
    reg.create(A(1));
    reg.create(B(2));
    reg.create(A(3), B(3));

    reg.each<>([](EntityId id){
        Log::info(id);
    });

    Log::info("");
    reg.each<A>([](EntityId id, A &a){
        Log::info(id, " ", a.a);
    });

    Log::info("");
    reg.each<B>([](EntityId id, B &b){
        Log::info(id, " ", b.b);
    });

    Log::info("");
    reg.each<A, B>([](EntityId id, A &a, B &b){
        Log::info(id, " ", a.a, " ", b.b);
    });
    return 0;
}
//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#include "tridot.h"

namespace tri {

    class Template : public Module {
    public:
        virtual void startup() override {
            
        }

        virtual void update() override {
        
        }
    };
    TRI_REGISTER_MODULE(Template);

    class TemplateComponent {
    public:
        float x;
        float y;
        float z;

        void update() {

        }
    };
    TRI_REGISTER_COMPONENT_3(TemplateComponent, x, y, z);
    TRI_REGISTER_COMPONENT_UPDATE(TemplateComponent, update);

}

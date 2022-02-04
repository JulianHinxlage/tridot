//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "Shader.h"
#include "core/util/Ref.h"

namespace tri {

    class ShaderState {
    public:
        template<typename T>
        void set(const std::string& uniform, T value) {
            for (auto& state : states) {
                if (state && state->name == uniform) {
                    state = std::static_pointer_cast<Base>(Ref<Value<T>>::make(uniform, value));
                    return;
                }
            }
            states.push_back(std::static_pointer_cast<Base>(Ref<Value<T>>::make(uniform, value)));
        }

        template<typename T>
        void set(const std::string& uniform, T *values, int count) {
            for (auto& state : states) {
                if (state && state->name == uniform) {
                    state = std::static_pointer_cast<Base>(Ref<Values<T>>::make(uniform, values, count));
                    return;
                }
            }
            states.push_back(std::static_pointer_cast<Base>(Ref<Values<T>>::make(uniform, values, count)));
        }

        void apply(Shader *shader) {
            for (auto& state : states) {
                if (state) {
                    state->apply(shader);
                }
            }
        }

    private:
        class Base {
        public:
            std::string name;
            int typeId = -1;
            virtual void apply(Shader* shader) = 0;

            virtual void* getData() = 0;
        };

        template<typename T>
        class Value : public Base {
        public:
            T value;

            Value(const std::string& name, T value) {
                this->name = name;
                this->value = value;
                typeId = env->reflection->getTypeId<T>();
            }
            
            void apply(Shader* shader) override {
                shader->set(name, value);
            }

            virtual void* getData() {
                return &value;
            }
        };

        template<typename T>
        class Values : public Base {
        public:
            std::vector<T> values;

            Values(const std::string& name, T* values, int count) {
                this->name = name;
                this->values.insert(this->values.begin(), values, values + count);
                typeId = env->reflection->getTypeId<std::vector<T>>();
            }

            void apply(Shader* shader) override {
                shader->set(name, values.data(), (int)values.size());
            }

            virtual void* getData() {
                return &values;
            }
        };

        std::vector<Ref<Base>> states;

    public:
        const std::vector<Ref<Base>>& getValues() { return states; }

    };

}
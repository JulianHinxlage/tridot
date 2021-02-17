//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#ifndef TRIDOT_SHADER_H
#define TRIDOT_SHADER_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

namespace tridot {

    class Shader {
    public:
        Shader();
        ~Shader();

        void bind();
        static void unbind();
        uint32_t getId();

        bool load(const std::string &file);
        bool preLoad(const std::string &file);
        bool postLoad();

        void set(const std::string &uniform, int value);
        void set(const std::string &uniform, float value);
        void set(const std::string &uniform, const glm::vec2 &value);
        void set(const std::string &uniform, const glm::vec3 &value);
        void set(const std::string &uniform, const glm::vec4 &value);
        void set(const std::string &uniform, const glm::mat2 &value);
        void set(const std::string &uniform, const glm::mat3 &value);
        void set(const std::string &uniform, const glm::mat4 &value);
        void set(const std::string &uniform, int *values, int count);
        void set(const std::string &uniform, float *values, int count);
        void set(const std::string &uniform, glm::vec2 *values, int count);
        void set(const std::string &uniform, glm::vec3 *values, int count);
        void set(const std::string &uniform, glm::vec4 *values, int count);
        void set(const std::string &uniform, glm::mat2 *values, int count);
        void set(const std::string &uniform, glm::mat3 *values, int count);
        void set(const std::string &uniform, glm::mat4 *values, int count);

    private:
        uint32_t id;
        std::vector<std::pair<uint32_t, std::string>> sources;
        std::unordered_map<std::string, uint32_t> locations;
        std::string file;

        uint32_t getLocation(const std::string &name);

    };

}

#endif //TRIDOT_SHADER_H
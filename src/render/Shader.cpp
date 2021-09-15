//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "Shader.h"
#include "core/core.h"
#include "GL/glew.h"
#include <fstream>

namespace tri {

    Shader::Shader() {
        id = 0;
    }

    Shader::~Shader() {
        if(id != 0){
            glDeleteProgram(id);
            env->console->trace("deleted shader ", id, " ", file);
            id = 0;
        }
    }

    void bindShader(uint32_t id){
        static uint32_t currentId = 0;
        if(id != currentId){
            glUseProgram(id);
            currentId = id;
        }
    }

    void Shader::bind() {
        bindShader(id);
    }

    void Shader::unbind() {
        bindShader(0);
    }

    uint32_t Shader::getId() {
        return id;
    }

    /*
    bool Shader::load(const std::string &file) {
        if(!preLoad(file)){
            return false;
        }
        return postLoad();
    }
     */

    bool Shader::load(const std::string &file) {
        std::ifstream stream(file);
        if(!stream.is_open()){
            env->console->warning("shader: file ", file, " not found");
            return false;
        }
        this->file = file;
        sources.clear();
        std::string line;
        while(std::getline(stream, line, '\n')){
            if(line == "#type vertex"){
                sources.push_back({(uint32_t)GL_VERTEX_SHADER, ""});
            }else if(line == "#type fragment"){
                sources.push_back({(uint32_t)GL_FRAGMENT_SHADER, ""});
            }else if(line == "#type geometry"){
                sources.push_back({(uint32_t)GL_GEOMETRY_SHADER, ""});
            }else if(line == "#type compute"){
                sources.push_back({(uint32_t)GL_COMPUTE_SHADER, ""});
            }else{
                if(!sources.empty()){
                    sources.back().second += line;
                    sources.back().second += '\n';
                }
            }
        }
        return sources.size() > 0;
    }

    bool Shader::loadActivate() {
        std::vector<GLuint> shaderIds;
        shaderIds.reserve(sources.size());
        for(auto &source : sources){
            GLuint shaderId = glCreateShader((GLenum)source.first);
            const char *src = source.second.c_str();
            int size = source.second.size();
            glShaderSource(shaderId, 1, &src, &size);
            glCompileShader(shaderId);
            GLint isCompiled = 0;
            glGetShaderiv(shaderId, GL_COMPILE_STATUS, &isCompiled);
            if(isCompiled == GL_FALSE){
                int logLength = 0;
                glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
                std::string log(logLength, '\0');
                glGetShaderInfoLog(shaderId, logLength, &logLength, log.data());
                if((GLenum)source.first == GL_VERTEX_SHADER){
                    env->console->warning("vertex shader compilation:\n", log);
                }else if((GLenum)source.first == GL_FRAGMENT_SHADER){
                    env->console->warning("fragment shader compilation:\n", log);
                }else if((GLenum)source.first == GL_GEOMETRY_SHADER){
                    env->console->warning("geometry shader compilation:\n", log);
                }else if((GLenum)source.first == GL_COMPUTE_SHADER){
                    env->console->warning("compute shader compilation:\n", log);
                }
                glDeleteShader(shaderId);
            }else{
                shaderIds.push_back(shaderId);
            }
        }

        if(sources.size() != shaderIds.size()){
            for(auto &shaderId : shaderIds){
                glDeleteShader(shaderId);
            }
            sources.clear();
            return false;
        }
        sources.clear();

        GLuint programId = glCreateProgram();
        for(auto &shaderId : shaderIds){
            glAttachShader(programId, shaderId);
        }
        glLinkProgram(programId);
        GLint isLinked = 0;
        glGetProgramiv(programId, GL_LINK_STATUS, &isLinked);
        if(isLinked == GL_FALSE){
            int logLength = 0;
            glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);
            std::string log(logLength, '\0');
            glGetProgramInfoLog(programId, logLength, &logLength, log.data());
            env->console->warning("shader linking:\n", log);

            for(auto &shaderId : shaderIds){
                glDetachShader(programId, shaderId);
                glDeleteShader(shaderId);
            }

            glDeleteProgram(programId);
            return false;
        }

        for(auto &shaderId : shaderIds){
            glDetachShader(programId, shaderId);
            glDeleteShader(shaderId);
        }
        id = programId;
        env->console->trace("created shader ", id);
        env->console->debug("loaded shader ", file);
        locations.clear();
        bufferLocations.clear();
        return true;
    }

    bool Shader::has(const std::string &uniform) {
        if(id == 0){
            return false;
        }else{
            return getLocation(uniform, false) != -1;
        }
    }

    void Shader::set(const std::string &uniform, int value) {
        glUniform1i(getLocation(uniform), value);
    }

    void Shader::set(const std::string &uniform, float value) {
        glUniform1f(getLocation(uniform), value);
    }

    void Shader::set(const std::string &uniform, const glm::vec2 &value) {
        glUniform2f(getLocation(uniform), value.x, value.y);
    }

    void Shader::set(const std::string &uniform, const glm::vec3 &value) {
        glUniform3f(getLocation(uniform), value.x, value.y, value.z);
    }

    void Shader::set(const std::string &uniform, const glm::vec4 &value) {
        glUniform4f(getLocation(uniform), value.x, value.y, value.z, value.w);
    }

    void Shader::set(const std::string &uniform, const glm::mat2 &value) {
        glUniformMatrix2fv(getLocation(uniform), 1, GL_FALSE, (float*)&value);
    }

    void Shader::set(const std::string &uniform, const glm::mat3 &value) {
        glUniformMatrix3fv(getLocation(uniform), 1, GL_FALSE, (float*)&value);
    }

    void Shader::set(const std::string &uniform, const glm::mat4 &value) {
        glUniformMatrix4fv(getLocation(uniform), 1, GL_FALSE, (float*)&value);
    }

    void Shader::set(const std::string &uniform, int *values, int count) {
        glUniform1iv(getLocation(uniform), count, values);
    }

    void Shader::set(const std::string &uniform, float *values, int count) {
        glUniform1fv(getLocation(uniform), count, values);
    }

    void Shader::set(const std::string &uniform, glm::vec2 *values, int count) {
        glUniform2fv(getLocation(uniform), count, (float*)values);
    }

    void Shader::set(const std::string &uniform, glm::vec3 *values, int count) {
        glUniform3fv(getLocation(uniform), count, (float*)values);
    }

    void Shader::set(const std::string &uniform, glm::vec4 *values, int count) {
        glUniform4fv(getLocation(uniform), count, (float*)values);
    }

    void Shader::set(const std::string &uniform, glm::mat2 *values, int count) {
        glUniformMatrix2fv(getLocation(uniform), count, GL_FALSE, (float*)values);
    }

    void Shader::set(const std::string &uniform, glm::mat3 *values, int count) {
        glUniformMatrix3fv(getLocation(uniform), count, GL_FALSE, (float*)values);
    }

    void Shader::set(const std::string &uniform, glm::mat4 *values, int count) {
        glUniformMatrix4fv(getLocation(uniform), count, GL_FALSE, (float*)values);
    }

    void Shader::set(const std::string &uniform, Buffer *buffer) {
        uint32_t location = getBufferLocation(uniform);
        if (location != -1) {
            glUniformBlockBinding(id, location, location);
            glBindBufferBase(GL_UNIFORM_BUFFER, location, buffer->getId());
        }
    }

    uint32_t Shader::getLocation(const std::string &name, bool warn) {
        auto entry = locations.find(name);
        if(entry != locations.end()){
            return entry->second;
        }else{
            uint32_t location = glGetUniformLocation(id, name.c_str());
            if(location == -1 && warn){
                env->console->warning("uniform ", name, " not found");
            }
            locations[name] = location;
            return location;
        }
    }

    uint32_t Shader::getBufferLocation(const std::string &name) {
        auto entry = bufferLocations.find(name);
        if(entry != bufferLocations.end()){
            return entry->second;
        }else{
            uint32_t location = glGetUniformBlockIndex(id, name.c_str());
            if(location == -1){
                env->console->warning("uniform buffer ", name, " not found");
            }
            bufferLocations[name] = location;
            return location;
        }
    }

}
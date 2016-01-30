#ifndef SHADERREGISTRY_H
#define SHADERREGISTRY_H

#include "common.h"
#include "shader.h"

struct ShaderRegistry {
	static std::map<std::string, std::shared_ptr<ShaderProgram>> programs;
	static std::map<std::string, Shader> shaders;

	static std::shared_ptr<ShaderProgram> CreateProgram(const std::string&, const std::string& vs, const std::string& fs);
	static std::shared_ptr<ShaderProgram> GetProgram(const std::string&);
};

#endif

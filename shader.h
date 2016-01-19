#ifndef SHADER_H
#define SHADER_H

#include "common.h"
#include <map>

struct Shader {
	u32 id = 0;
	std::string filename;

	Shader(const string& fname);
	~Shader();
	void Compile();
};

struct ShaderProgram {
	u32 id = 0;
	
	std::map<string, s32> attributes;
	std::map<string, s32> uniforms;

	ShaderProgram();
	~ShaderProgram();

	void Attach(const Shader&);
	void Link();
	void Use();

	s32 GetAttribute(const string&);
	s32 GetUniform(const string&);
};

#endif
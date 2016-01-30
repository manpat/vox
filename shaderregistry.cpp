#include "shaderregistry.h"

static Log logger{"ShaderRegistry"};

std::map<std::string, std::shared_ptr<ShaderProgram>> ShaderRegistry::programs;
std::map<std::string, Shader> ShaderRegistry::shaders;

std::shared_ptr<ShaderProgram> ShaderRegistry::CreateProgram(const std::string& name,
	const std::string& vsname, const std::string& fsname) {

	std::shared_ptr<ShaderProgram>& program = programs[name];
	if(program) {
		logger << "Name conflict when creating program " << name;
		return program;
	}

	Shader* vs = &shaders[vsname];
	if(!*vs) {
		vs->filename = vsname;
		vs->Compile();
	}

	Shader* fs = &shaders[fsname];
	if(!*fs) {
		fs->filename = fsname;
		fs->Compile();
	}

	program = std::make_shared<ShaderProgram>();
	program->Attach(*vs);
	program->Attach(*fs);
	program->Link();
	return program;
}

std::shared_ptr<ShaderProgram> ShaderRegistry::GetProgram(const std::string& name) {
	auto it = programs.find(name);
	if(it == programs.end()) {
		throw "Unknown shader program: " + name;
	}

	return it->second;
}

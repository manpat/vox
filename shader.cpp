#include "shader.h"
#include <fstream>

static Log shaderLog{"Shader"};

Shader::Shader(const string& f) : id{0}, filename{f} {}

Shader::~Shader() {
	if(id) {
		glDeleteShader(id);
		id = 0;
	}
}


void Shader::Compile() {
	auto ext = filename.substr(filename.size()-2);

	u32 type = 0;
	if(ext == "vs") type = GL_VERTEX_SHADER;
	else if(ext == "fs") type = GL_FRAGMENT_SHADER;
	else {
		shaderLog << "Unknown file extension: " << ext;
		return;
	}

	id = glCreateShader(type);

	std::string contents;
	std::ifstream file{filename, std::ifstream::binary};

	if(!file) {
		shaderLog << "File open failed for " << filename;
		return;
	}

	file.seekg(0, file.end);
	contents.resize(file.tellg());
	file.seekg(0, file.beg);

	file.read(&contents[0], contents.size());
	file.close();

	std::vector<const char*> src { contents.data() };
	std::vector<s32> srcLen { (s32)contents.size() };

	glShaderSource(id, src.size(), src.data(), srcLen.data());
	glCompileShader(id);

	s32 shaderStatus = 0;
	glGetShaderiv(id, GL_COMPILE_STATUS, &shaderStatus);
	if(!shaderStatus) {
		s32 infoLogLength = 0;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);

		char* strInfoLog = new char[infoLogLength + 1];
		glGetShaderInfoLog(id, infoLogLength, nullptr, strInfoLog);

		Log() << "Compilation of " << filename << " failed: " << strInfoLog;
		delete[] strInfoLog;

		glDeleteShader(id);
		id = 0;
	}
}

ShaderProgram::ShaderProgram() {
	id = glCreateProgram();
}

ShaderProgram::~ShaderProgram() {
	glDeleteProgram(id);
}

void ShaderProgram::Attach(const Shader& sh) {
	glAttachShader(id, sh.id);
}

void ShaderProgram::Link() {
	glLinkProgram(id);

	s32 linkStatus;
	glGetProgramiv(id, GL_LINK_STATUS, &linkStatus);
	if (!linkStatus) {
		shaderLog << "Shader program linking failed";
		return;
	}
}

void ShaderProgram::Use() {
	glUseProgram(id);
}

s32 ShaderProgram::GetAttribute(const string& n) {
	auto it = attributes.find(n);
	if(it == attributes.end()) {
		s32 a = glGetAttribLocation(id, n.data());
		attributes[n] = a;
		if(a == -1) {
			shaderLog << "Attribute '" << n << "' not found";
		}
		return a;
	}

	return it->second;
}

s32 ShaderProgram::GetUniform(const string& n) {
	auto it = uniforms.find(n);
	if(it == uniforms.end()) {
		s32 a = glGetUniformLocation(id, n.data());
		uniforms[n] = a;
		if(a == -1) {
			shaderLog << "Uniform '" << n << "' not found";
		}
		return a;
	}

	return it->second;
}

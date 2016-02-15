#include "common.h"
#include "client.h"

#include <glm/gtx/string_cast.hpp>

std::ostream& operator<<(std::ostream& o, const vec2& v) {
	return o << glm::to_string(v);
}
std::ostream& operator<<(std::ostream& o, const vec3& v) {
	return o << glm::to_string(v);
}
std::ostream& operator<<(std::ostream& o, const vec4& v) {
	return o << glm::to_string(v);
}

std::ostream& operator<<(std::ostream& o, const mat3& v) {
	return o << glm::to_string(v);
}
std::ostream& operator<<(std::ostream& o, const mat4& v) {
	return o << glm::to_string(v);
}

static Log logger{"Main"};

s32 main() {
	try {
		Client client{};
		client.Init();
		client.Run();

	} catch(const std::string& s) {
		logger << "Exception! " << Log::NL << s;

	} catch(const char* s) {
		logger << "Exception! " << Log::NL << s;

	} catch(const std::exception& e) {
		logger << "Exception! " << Log::NL << e.what();
	}

	return 0;
}

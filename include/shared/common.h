#ifndef COMMON_H
#define COMMON_H

#include <map>
#include <cmath>
#include <vector>
#include <string>
#include <memory>
#include <ostream>
#include <cstdint>
#include <algorithm>

#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifdef VOXCLIENT
#	define GL_GLEXT_PROTOTYPES
#	include <SDL2/SDL_opengl.h>
#endif

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using f32 = float;
using f64 = double;

using std::string;

using glm::vec2;
using glm::vec3;
using glm::ivec2;
using glm::ivec3;
using glm::vec4;

using glm::mat3;
using glm::mat4;

using glm::quat;

#ifndef M_PI
#define M_PI 3.14159265359
#endif

#ifndef PI
#define PI M_PI
#endif

std::ostream& operator<<(std::ostream& o, const vec2&);
std::ostream& operator<<(std::ostream& o, const vec3&);
std::ostream& operator<<(std::ostream& o, const vec4&);

std::ostream& operator<<(std::ostream& o, const mat3&);
std::ostream& operator<<(std::ostream& o, const mat4&);

#include "log.h"
#include "apptime.h"

#endif
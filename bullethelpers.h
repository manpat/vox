#ifndef BULLETHELPERS_H
#define BULLETHELPERS_H

#include "common.h"
#include <LinearMath/btVector3.h>
#include <LinearMath/btQuaternion.h>

inline btVector3 o2bt(const vec3& v){
	return {v.x, v.y, v.z};
}
inline vec3 bt2o(const btVector3& v){
	return vec3{v.x(), v.y(), v.z()};
}

inline btQuaternion o2bt(const quat& o){
	return {o.x, o.y, o.z, o.w};
}
inline quat bt2o(const btQuaternion& o){
	return quat{o.w(), o.x(), o.y(), o.z()};
}

#endif
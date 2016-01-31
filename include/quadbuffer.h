#ifndef QUADBUFFER_H
#define QUADBUFFER_H

#include "common.h"

// An element buffer that remaps quad data to triangles
struct QuadElementBuffer {
	static u32 ebo;
	static u64 count;
	static s32 elementType;

	static void SetNumQuads(u64);
	static void Draw(u64);
};


#endif
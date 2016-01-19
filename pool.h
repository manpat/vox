#ifndef POOL_H
#define POOL_H

#include "common.h"

// Frame pool is a double buffered memory pool for POD
//	Buffers are swapped once per frame.
//	Allocated objects are valid for at least one frame after allocation.
// 	Destructors are not called on freeing.
struct FramePool {
	u8* buffers[2];
	u32 size;
	u32 idx;
	u8 currentBuffer;

	FramePool(u32 size);
	~FramePool();

	template<class C, class... A>
	C* New(A...);

	void Update();
};

#include "pool.inl"

#endif
#include "pool.h"

FramePool::FramePool(u32 _size) : size{_size}, idx{0}, currentBuffer{0} {
	buffers[0] = new u8[size];
	buffers[1] = new u8[size];

	if(!buffers[0] || !buffers[1]) throw "FramePool memory allocation failed";
}

FramePool::~FramePool(){
	delete[] buffers[0];
	delete[] buffers[1];
	buffers[0] = nullptr;
	buffers[1] = nullptr;
}

void FramePool::Update(){
	idx = 0; // Reset
	currentBuffer = 1-currentBuffer; // Swap
}
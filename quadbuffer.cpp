#include "quadbuffer.h"

u32 QuadElementBuffer::ebo = 0;
u64 QuadElementBuffer::count = 0;
s32 QuadElementBuffer::elementType = GL_UNSIGNED_BYTE;

void QuadElementBuffer::SetNumQuads(u64 numQuads) {
	using std::numeric_limits;
	if(numQuads < count) return;

	if(numQuads*6u >= numeric_limits<u16>::max()) elementType = GL_UNSIGNED_INT;
	else if(numQuads*6u >= numeric_limits<u8>::max()) elementType = GL_UNSIGNED_SHORT;

	if(!ebo) {
		glGenBuffers(1, &ebo);
		count = numQuads << 1;
	}else{
		while(numQuads > count)
			count <<= 1;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	switch(elementType) {
		case GL_UNSIGNED_BYTE: {
			u8* elements = new u8[count*6];
			for(u64 i = 0; i < count; i++) {
				elements[i*6+0] = i*4+0;
				elements[i*6+1] = i*4+2;
				elements[i*6+2] = i*4+1;

				elements[i*6+3] = i*4+0;
				elements[i*6+4] = i*4+3;
				elements[i*6+5] = i*4+2;
			}
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, count*6*sizeof(u8), elements, GL_STATIC_DRAW);
			delete[] elements;
		} break;

		case GL_UNSIGNED_SHORT: {
			u16* elements = new u16[count*6];
			for(u64 i = 0; i < count; i++) {
				elements[i*6+0] = i*4+0;
				elements[i*6+1] = i*4+2;
				elements[i*6+2] = i*4+1;

				elements[i*6+3] = i*4+0;
				elements[i*6+4] = i*4+3;
				elements[i*6+5] = i*4+2;
			}
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, count*6*sizeof(u16), elements, GL_STATIC_DRAW);
			delete[] elements;
		} break;

		case GL_UNSIGNED_INT: {
			u32* elements = new u32[count*6];
			for(u64 i = 0; i < count; i++) {
				elements[i*6+0] = i*4+0;
				elements[i*6+1] = i*4+2;
				elements[i*6+2] = i*4+1;

				elements[i*6+3] = i*4+0;
				elements[i*6+4] = i*4+3;
				elements[i*6+5] = i*4+2;
			}
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, count*6*sizeof(u32), elements, GL_STATIC_DRAW);
			delete[] elements;
		} break;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void QuadElementBuffer::Draw(u64 numQuads) {
	SetNumQuads(numQuads);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glDrawElements(GL_TRIANGLES, numQuads*6, elementType, nullptr);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

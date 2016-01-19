#include <type_traits>
#include "typehelpers.h"

template<class C, class... A>
C* FramePool::New(A... a) {
	if(sizeof(C)+idx >= size) throw "FramePool buffer overflow";

	if(!std::is_trivially_destructible<C>::value) {
		throw std::string{"Type "} + getTypeName<C>() + " created in FramePool\nThis could cause leaks";
	}

	auto mem = new (&buffers[currentBuffer][idx]) C{a...};
	idx += sizeof(C);
	return mem;
}
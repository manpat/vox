#ifndef TYPEHELPERS_H
#define TYPEHELPERS_H

#include <string>
#include <typeinfo>
#include "common.h"

#ifdef __GNUC__
// This is for fancy type demangling in GCC
#	include <cxxabi.h>

template<class T>
std::string getTypeName() {
	static char nameBuff[100];
	static s32 status = 0;
	size_t buffLength = 100;

	return std::string{abi::__cxa_demangle(typeid(T).name(), nameBuff, &buffLength, &status)};
}

#else

template<class T>
std::string getTypeName() {
	return std::string{typeid(T).name()};
}

#endif

#endif
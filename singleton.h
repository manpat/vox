#ifndef SINGLETON_H
#define SINGLETON_H

#include "typehelpers.h"

template<class C>
struct Singleton {
protected:
	static C* instance;

public:
	Singleton(){
		if(instance)
			throw "Tried to instantiate more than one " + getTypeName<C>();

		instance = static_cast<C*>(this);
	}
	~Singleton(){
		instance = nullptr;
	}

	static C* GetSingleton(){
		return instance;
	}
};

#endif
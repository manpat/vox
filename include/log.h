#ifndef LOG_H
#define LOG_H

#include <string>
#include <sstream>
#include <fstream>
#include <utility>
#include <type_traits>

// [time] [system]\tFormatted string

struct Log {
	struct Proxy {
		Log* log;

		Proxy(Log*);
		Proxy(Proxy&&);
		~Proxy();

		Proxy& PrintStamp();

		template<class T>
		Proxy& Print(T&& val);
		Proxy& Print(void (*)(Proxy&));

		template<class T>
		Proxy& operator<<(T&& val);
	};

	static std::ofstream& GetLogFile();

	// Members
	const char* systemName = nullptr;
	std::ostringstream ss;

	// Functions
	Log(const char* = "General");

	template<class T>
	Proxy Print(T&& val);

	template<class T>
	Proxy operator<<(T&& val);

	// Modifiers
	template<int width = 4>
	static void Tab(Proxy&);
	static void NL(Proxy&);
};

#include "log.inl"

#endif
#include "common.h"
#include "log.h"
#include <iostream>
#include <cstring>
#include <ctime>

Log::Log(const char* sys) : systemName{sys} {}

std::ofstream& Log::GetLogFile() {
	static std::ofstream file("out");
	return file;
}

// Proxy

Log::Proxy::Proxy(Log* l) : log{l} {
	log->ss.str("");
	PrintStamp();
}

Log::Proxy::Proxy(Proxy&& p) : log{p.log} {
	p.log = nullptr;
}

Log::Proxy::~Proxy() {
	if(!log) return;

	auto str = log->ss.str();

	std::cout << str << std::endl;
	GetLogFile() << str << std::endl;

	log->ss.str("");
}

auto Log::Proxy::PrintStamp() -> Proxy& {
	if(log) {
		static char stampBuffer[128];

		time_t rawtime;
		std::time(&rawtime);
		auto ti = std::localtime(&rawtime);

		s32 len = strlen(log->systemName);

		s32 stampLen = snprintf(stampBuffer, 128, 
			"[%.2d%.2d%.4d %.2d%.2d%.2d] [%.*s]", 
			ti->tm_year+1900, ti->tm_mon, ti->tm_mday, 
			ti->tm_hour, ti->tm_min, ti->tm_sec,
			len, log->systemName);

		Print(stampBuffer);

		s32 padding = std::max(40 - stampLen, 1);
		Print(std::string(padding, ' '));
	}

	return *this;
}

// Apply modifier
auto Log::Proxy::Print(void (*func)(Proxy&)) -> Proxy& {
	(*func)(*this);
	return *this;
}

// Modifiers

void Log::NL(Proxy& proxy) {
	proxy.log->ss << "\n";
	proxy.PrintStamp();
}

template<>
void Log::Tab<0>(Proxy&) {}

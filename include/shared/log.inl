
// Log

template<class T>
auto Log::Print(T&& val) -> Proxy {
	return std::move(Proxy{this}.Print(std::forward<T>(val)));
}

template<class T>
auto Log::operator<<(T&& val) -> Proxy {
	return Print(std::forward<T>(val));
}

// Log::Proxy

template<class T>
auto Log::Proxy::Print(T&& val) -> Proxy& {
	if(log) log->ss << val;
	return *this;
}

template<class T>
auto Log::Proxy::operator<<(T&& val) -> Proxy& {
	return Print(std::forward<T>(val));
}

// Modifiers

template<int width>
void Log::Tab(Proxy& proxy) {
	auto& ss = proxy.log->ss;
	size_t curr = ss.tellp();

	ss << std::string(width - (curr % width), ' ');
}
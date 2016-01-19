#include "common.h"
#include "app.h"

static Log logger{"Main"};

s32 main() {
	try {
		App app{};
		app.Init();
		app.Run();

	} catch(const std::string& s) {
		logger << "Exception! " << Log::NL << s;

	} catch(const char* s) {
		logger << "Exception! " << Log::NL << s;

	} catch(const std::exception& e) {
		logger << "Exception! " << Log::NL << e.what();
	}

	return 0;
}
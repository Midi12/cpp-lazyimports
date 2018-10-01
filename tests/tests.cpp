#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../src/cpp-lazyimports.hpp"

// compile with `clang++ --std=c++17 -Xclang -flto-visibility-public-std -Wall -Wextra -o bin/tests.exe tests/tests.cpp`

#if defined(_WIN64)
	#define MODULE "shared.dll"
	#define FUNC "?getpid@@YAKXZ"
	#define MODULEFUNCNOARG "shared.dll!?getpid@@YAKXZ"
	#define MODULEFUNCWITHARG "shared.dll!?sum@@YAHHH@Z"
#elif defined(__linux__) or defined(__APPLE__)
	#define MODULE "./shared.so"
	#define FUNC "GetCurrentProcessId"
	#define MODULEFUNCNOARG "./shared.so!GetCurrentProcessId"
	#define MODULEFUNCWITHARG "./shared.so!sum"
#else
	#error platform not supported
#endif

TEST_CASE("Module is loaded", "[lazymodule]") {
	cpp_lazyimports::lazymodule mod;

	SECTION("Checking initial state") {
		REQUIRE(mod.handle() == 0);
		REQUIRE(mod.name().empty());
		REQUIRE(mod.name().size() == 0);
	}

	SECTION("Loading the module and checking data") {
		cpp_lazyimports::lazymodulecollection::instance().find_or_load(MODULE, mod);
		
		REQUIRE(mod.handle() != 0);
		REQUIRE(!mod.name().empty());
		REQUIRE(mod.name().size() > 0);
	}
}

#define REGISTER_IMPORT(Path) \
	::cpp_lazyimports::lazymodulecollection::instance().register_import(Path)

TEST_CASE("Module export is resolved", "[lazyimport]") {
	cpp_lazyimports::lazyimport imp = REGISTER_IMPORT(MODULEFUNCNOARG);

	SECTION("Checking import data") {
		REQUIRE(imp.ptr() != 0);
		REQUIRE(!imp.name().empty());
		REQUIRE(imp.name().size() > 0);
	}
}

TEST_CASE("Module export can be called (void)", "[lazyimport]") {
	cpp_lazyimports::lazyimport imp = REGISTER_IMPORT(MODULEFUNCNOARG);

	SECTION("Checking import data") {
		REQUIRE(imp.ptr() != 0);
		REQUIRE(!imp.name().empty());
		REQUIRE(imp.name().size() > 0);
	}

	SECTION("Calling function without arguments") {
#if defined(_WIN64)
		REQUIRE(imp.call<DWORD>() == GetCurrentProcessId());
#elif defined(__linux__) or defined(__APPLE__)
		REQUIRE(imp.call<pid_t>() == getpid());
#endif
	}
}

TEST_CASE("Module export can be called (params)", "[lazyimport]") {
	cpp_lazyimports::lazyimport imp = REGISTER_IMPORT(MODULEFUNCWITHARG);

	SECTION("Checking import data") {
		REQUIRE(imp.ptr() != 0);
		REQUIRE(!imp.name().empty());
		REQUIRE(imp.name().size() > 0);
	}

	SECTION("Calling function with arguments") {
		REQUIRE(imp.call<int>(1337, 42) == (1337 + 42));
	}
}

TEST_CASE("Modules and imports are loaded only once, next loads returns same reference", "[lazymodule & lazyimport]") {
	cpp_lazyimports::lazymodule mod0, mod1;
	cpp_lazyimports::lazymodulecollection::instance().find_or_load(MODULE, mod0);
	cpp_lazyimports::lazymodulecollection::instance().find_or_load(MODULE, mod1);

	SECTION("Checking modules refs & hashes") {
		REQUIRE(mod0.handle() == mod1.handle());
		REQUIRE(mod0.hash() == mod1.hash());
		REQUIRE(cpp_lazyimports::lazymodulecollection::instance().size() == 1);
	}
	
	cpp_lazyimports::lazyimport imp0, imp1;
	mod0.add(FUNC, imp0);
	mod1.add(FUNC, imp1);
	
	SECTION("Checking imports refs & hashes") {
		REQUIRE(imp0.ptr() == imp1.ptr());
		REQUIRE(imp0.hash() == imp1.hash());
		REQUIRE(mod0.imports().size() == 1);
		REQUIRE(mod1.imports().size() == 1);
	}
}
#include "../src/cpp-lazyimports.hpp"

#include <cassert>
#include <iostream>

#if defined(_WIN64)
#include <windows.h>
#elif defined(__linux__) or defined(__APPLE__)
#include <unistd.h>
#else
	#error platform not supported
#endif

// compile with `clang++ --std=c++17 -Xclang -flto-visibility-public-std -Wall -Wextra -o bin/example.exe docs/example.cpp`

using namespace cpp_lazyimports::literals;

int main(int /*argc*/, char ** /*argv*/, char ** /*envp*/) {

#if defined(_WIN64)
	DWORD a, b, c;

	try {
		auto getpid = "shared.dll!?getpid@@YAKXZ"_lazy;

		using fngetpid_t = DWORD (__stdcall *)(void);
		fngetpid_t fngetpid = reinterpret_cast<fngetpid_t>(getpid.ptr());
		a = fngetpid();

		b = getpid.operator()<DWORD>();

		c = LAZYCALL(DWORD, "shared.dll!?getpid@@YAKXZ");
	} catch (cpp_lazyimports::exceptions::lazymoduleexception &ex) {
		std::cout << ex.what() << " " << ex.last_error() << std::endl;
	} catch (cpp_lazyimports::exceptions::lazyimportexception &ex) {
		std::cout << ex.what() << " " << ex.last_error() << std::endl;
	}

#elif defined(__linux__) or defined(__APPLE__)
	pid_t a, b, c;
	
	try {
		auto getcurrentprocessid = "./shared.so!GetCurrentProcessId"_lazy;

		using fnGetCurrentProcessId_t = pid_t (*)(void);
		fnGetCurrentProcessId_t fngetcurrentprocessid = reinterpret_cast<fnGetCurrentProcessId_t>(getcurrentprocessid.ptr());
		a = fngetcurrentprocessid();

		b = getcurrentprocessid.operator()<pid_t>();

		c = LAZYCALL(pid_t, "./shared.so!GetCurrentProcessId");
	} catch (cpp_lazyimports::exceptions::lazymoduleexception &ex) {
		std::cout << ex.what() << " " << ex.last_error() << std::endl;
	} catch (cpp_lazyimports::exceptions::lazyimportexception &ex) {
		std::cout << ex.what() << " " << ex.last_error() << std::endl;
	}
#else
	#error platform not supported
#endif

	std::cout << "result " << a << " " << b << " " << c << std::endl;

	assert((a == c) && (a == b) && (b == c));

	std::cout << "good boy" << std::endl;

	return 0;
}
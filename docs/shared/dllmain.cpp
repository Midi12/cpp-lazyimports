#include "dllmain.hpp"

// compile with `clang++ -std=c++17 -shared -Wall -Wextra -lkernel32 -o bin/shared.dll docs/shared/dllmain.cpp`

BOOL WINAPI DllMain(HINSTANCE /*hinstDLL*/, DWORD /*fdwReason*/, LPVOID /*lpvReserved*/) {
	return TRUE;
}

__declspec(dllexport) DWORD getpid() {
	return ::GetCurrentProcessId();
}

__declspec(dllexport) int sum(int a, int b) {
	return a + b;
}
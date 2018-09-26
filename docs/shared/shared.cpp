#include "shared.hpp"

// compile with `clang++ -shared -fpic -o bin/shared.so docs/shared/shared.cpp`

extern "C" pid_t GetCurrentProcessId() {
	return getpid();
}


extern "C" int sum(int a, int b) {
	return a + b;
}
// FLAGS: -d0
// OUTPUT: \[jit\] Invalid or unsupported instruction at address 00000038:
// OUTPUT: \[jit\]   => jrel 0
// OUTPUT: \[jit\] Compilation failed

#include "test"

public OnGameModeInit() {
	#emit jrel 0
}

main() {
	TestExit();
}

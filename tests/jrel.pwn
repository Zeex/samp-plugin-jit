// FLAGS: -d0
// OUTPUT: \[jit\] Invalid or unsupported instruction at address 0000000c:
// OUTPUT: \[jit\]   => jrel 0
// OUTPUT: \[jit\] Compilation failed

#include "test"

main() {
	#emit jrel 0
}

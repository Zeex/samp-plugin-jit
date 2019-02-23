// OUTPUT: main\(\)
// OUTPUT: Error while executing main: Forced exit \(1\)

#include "test"

f() {
	#emit halt 1
}

main() {
	print("main()");
	f();
	print("FAIL");
}

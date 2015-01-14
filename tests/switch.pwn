// OUTPUT: default
// OUTPUT: default
// OUTPUT: case 1
// OUTPUT: case 2
// OUTPUT: case 3
// OUTPUT: default
// OUTPUT: case 10
// OUTPUT: default
// OUTPUT: case 15
// OUTPUT: default
// OUTPUT: default

#include <a_samp>
#include "test"

TestSwitch(x) {
	switch(x) {
		case 1: print("case 1");
		case 2: print("case 2");
		case 3: print("case 3");
		case 10: print("case 10");
		case 15: print("case 15");
		default: print("default");
	}
}

main() {
	TestSwitch(0);
	TestSwitch(-1);
	TestSwitch(1);
	TestSwitch(2);
	TestSwitch(3);
	TestSwitch(7);
	TestSwitch(10);
	TestSwitch(13);
	TestSwitch(15);
	TestSwitch(16);
	TestSwitch(30);
	TestExit();
}

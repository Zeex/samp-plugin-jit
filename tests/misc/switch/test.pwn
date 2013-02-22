#include <a_samp>
#include <test>

test_switch(x) {
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
	test_switch(0);
	test_switch(-1);
	test_switch(1);
	test_switch(2);
	test_switch(3);
	test_switch(7);
	test_switch(10);
	test_switch(13);
	test_switch(15);
	test_switch(16);
	test_switch(30);
	TestExit();
}

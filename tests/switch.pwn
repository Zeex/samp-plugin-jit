#include "test"

DoSwitch(x) {
	switch (x) {
		case 1:  return 1;
		case 2:  return 2;
		case 3:  return 3;
		case 10: return 10;
		case 15: return 15;
	}
	return 0;
}

main() {
	TEST_TRUE(DoSwitch(0) == 0);
	TEST_TRUE(DoSwitch(-1) == 0);
	TEST_TRUE(DoSwitch(1) == 1);
	TEST_TRUE(DoSwitch(2) == 2);
	TEST_TRUE(DoSwitch(3) == 3);
	TEST_TRUE(DoSwitch(7) == 0);
	TEST_TRUE(DoSwitch(10) == 10);
	TEST_TRUE(DoSwitch(13) == 0);
	TEST_TRUE(DoSwitch(15) == 15);
	TEST_TRUE(DoSwitch(16) == 0);
	TEST_TRUE(DoSwitch(30) == 0);
}

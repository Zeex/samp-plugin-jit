#include "test"

retNumargs(...)
{
	return numargs();
}

main() {
	TEST_TRUE(retNumargs() == 0);
	TEST_TRUE(retNumargs(1) == 1);
	TEST_TRUE(retNumargs(1, 2) == 2);
	TEST_TRUE(retNumargs(1, 2, 3) == 3);
	TEST_TRUE(retNumargs(1, 2, 3, 4) == 4);
	TEST_TRUE(retNumargs(1, 2, 3, 4, 5) == 5);
}


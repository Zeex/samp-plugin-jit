#include "float_const"
#include "test"

main() {
	TEST_TRUE(floatsqroot(0.0) == 0.0);
	TEST_TRUE(floatsqroot(1.0) == 1.0);
	TEST_TRUE(floatsqroot(4.0) == 2.0);
	TEST_TRUE(floatsqroot(9.0) == 3.0);
	TEST_TRUE(floatsqroot(16.0) == 4.0);
	TEST_TRUE(floatsqroot(25.0) == 5.0);
	TEST_TRUE(floatsqroot(POS_INF) == POS_INF);
}

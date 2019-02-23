#include "test"

main()
{
	new a[1 char];
    a{0} = 1;
	a{1} = 2;
	a{2} = 3;
	a{3} = 4;

	TEST_TRUE(a{0} == 1);
	TEST_TRUE(a{1} == 2);
	TEST_TRUE(a{2} == 3);
	TEST_TRUE(a{3} == 4);
}

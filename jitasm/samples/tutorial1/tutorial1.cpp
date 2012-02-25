#include "stdio.h"
#include "jitasm.h"

struct tutorial1 : jitasm::function<int, tutorial1, int>
{
	Result main(Reg32 a)
	{
		add(a, 1);
		return a;
	}
};

int main()
{
	// Make function instance
	tutorial1 f;

	// Runtime code genaration and run
	int result = f(99);

	printf("Result : %d\n", result);

	return 0;
}

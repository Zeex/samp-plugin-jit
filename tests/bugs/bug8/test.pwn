#include <a_samp>

native jit_exit(code = 0);

main()
{
	new a[1 char];
    a{0} = 1;
	a{1} = 2;
	a{2} = 3;
	a{3} = 4;
    printf("%d %d %d %d", a{0}, a{1}, a{2}, a{3});

	jit_exit();
}

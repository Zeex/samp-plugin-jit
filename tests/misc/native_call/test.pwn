#include <a_samp>

native jit_exit(code = 0);

main() {
	printf("%s%s%s%s", "it", " ", "works", "!");
	printf("%d", strlen("test")); // should output 4

	jit_exit();
}

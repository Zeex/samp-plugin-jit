// OUTPUT: it works!
// OUTPUT: 4

#include <a_samp>
#include <test>

main() {
	printf("%s%s%s%s", "it", " ", "works", "!");
	printf("%d", strlen("test")); // should output 4

	TestExit();
}

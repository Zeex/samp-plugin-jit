#include <a_samp>
#include <test>

Test() {
	printf("%f", VectorSize(0.0, 0.0, 0.0));
	printf("%f", VectorSize(5.0, 0.0, 0.0));
	printf("%f", VectorSize(0.0, 5.0, 0.0));
	printf("%f", VectorSize(0.0, 0.0, 5.0));
	printf("%f", VectorSize(1.0, 1.0, 1.0));
}

main() {
	Test();
	TestExit();
}

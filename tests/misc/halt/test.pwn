#include <a_samp>
#include <test>

main() {
	f();
	print("FAIL");
}

f() {
	#emit halt 1
}

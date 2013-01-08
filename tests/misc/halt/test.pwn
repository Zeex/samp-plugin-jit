#include <a_samp>

main() {
	f();
	print("FAIL");
}

f() {
	#emit halt 1
}

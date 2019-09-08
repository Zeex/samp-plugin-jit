// FLAGS: -d0
// OUTPUT: main
// OUTPUT: Running indefinitely because ProcessTick\(\) was requested
// OUTPUT: \[sleep\] Executing sleep_callback
// OUTPUT: \[sleep\] before exec: frm = 4264
// OUTPUT: \[sleep\] before exec: stk = 4270
// OUTPUT: \[sleep\] before exec: hea = 274
// OUTPUT: sleep_callback
// OUTPUT: Wake me after after 1 second please
// OUTPUT: \[sleep\] Scheduled continuation at \+500 ms
// OUTPUT: \[sleep\] after exec: frm = 4264
// OUTPUT: \[sleep\] after exec: stk = 4234
// OUTPUT: \[sleep\] after exec: hea = 274
// OUTPUT: \[sleep\] Continuing execution
// OUTPUT: \[sleep\] cip = 1e8
// OUTPUT: \[sleep\] pri = c0ffee
// OUTPUT: \[sleep\] alt = 0
// OUTPUT: \[sleep\] frm = 4264
// OUTPUT: \[sleep\] stk = 4234
// OUTPUT: \[sleep\] hea = 274
// OUTPUT: \[sleep\] reset_stk = 4270
// OUTPUT: \[sleep\] reset_hea = 274
// OUTPUT: I'm awake!
// OUTPUT: All tests passed

#include "test"

native schedule_continue(numMilliseconds);

forward sleep_callback();

main() {
	print("main");
}

public sleep_callback() {
	print("sleep_callback");

	new x = 0xc0ffee;
	new s[] = "I'm awake!";

	print("Wake me after after 1 second please");
	schedule_continue(500);
	sleep x;

	TEST_TRUE(x == 0xc0ffee);
	print(s);

	TestExit();
}

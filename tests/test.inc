#include <a_samp>

stock bool:test_false = false;
stock test_id = 0;

#define TEST_TRUE(%0) \
     do { \
          test_id++; \
          if (!(%0)) { \
               TestFailed(test_id, "Expression is false"); \
          } \
     } while (test_false)

#define TEST_FALSE(%0) \
     do { \
          test_id++; \
          if (%0) { \
               TestFailed(test_id, "Expression is true"); \
          } \
     } while (test_false)

stock TestFailed(id, const reason[]) {
     printf("Test #%d failed: %s", id, reason);
}

stock TestExit() {
     SendRconCommand("exit");
}

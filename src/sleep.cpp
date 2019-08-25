#include <chrono>
#include <cstdlib>
#include <vector>
#include "plugin.h"

/******************************************************************************

#include <a_samp>

native do_sleep();
native schedule_continue(numMilliseconds);

forward sleep_callback();

main() {
	print("main");
	SetTimer("sleep_callback", 1000, false);
}

public sleep_callback() {
	new x = 123;

	schedule_continue(1000);
	#emit break
	do_sleep();
	#emit break

	printf("Hello! x = %d", x);
	print("Right?");
}

******************************************************************************/

typedef void (*logprintf_t)(const char *format, ...);

static logprintf_t logprintf;
extern void *pAMXFunctions;

namespace sleep {
  using std::chrono::system_clock;
  using std::chrono::milliseconds;

  class ContinueTimer
  {
  public:
    ContinueTimer(AMX *amx, cell numMilliseconds):
      amx_(amx),
      scheduledTime_(system_clock::now() + milliseconds(numMilliseconds)),
      didExecute_(false)
    {}

    system_clock::time_point GetScheduledTime() const {
      return scheduledTime_;
    }

    int Execute() {
      logprintf("[sleep] Continuing execution");
      logprintf("[sleep] CIP = %x", amx_->cip);
      logprintf("[sleep] PRI = %x", amx_->pri);
      logprintf("[sleep] ALT = %x", amx_->alt);
      logprintf("[sleep] FRM = %x", amx_->frm);
      logprintf("[sleep] STK = %x", amx_->stk);
      logprintf("[sleep] HEA = %x", amx_->hea);
      didExecute_ = true;
      return amx_Exec(amx_, nullptr, AMX_EXEC_CONT);
    }

    bool DidExecute() const {
      return didExecute_;
    }

  private:
    AMX *amx_;
    system_clock::time_point scheduledTime_;
    bool didExecute_;
  };

  std::vector<sleep::ContinueTimer> continueTimers;

  void AddTimer(AMX *amx, cell numMilliseconds) {
    continueTimers.push_back(ContinueTimer(amx, numMilliseconds));
  }

  void ProcessTimers() {
    auto now = system_clock::now();
    for (auto &timer : continueTimers) {
      if (!timer.DidExecute() && now >= timer.GetScheduledTime()) {
        timer.Execute();
      }
    }
  }
}

static cell AMX_NATIVE_CALL n_do_sleep(AMX *amx, cell *params) {
  logprintf("[sleep] Sleep!");
  logprintf("[sleep] CIP = %x", amx->cip);
  logprintf("[sleep] FRM = %x", amx->frm);
  logprintf("[sleep] STK = %x", amx->stk);
  logprintf("[sleep] HEA = %x", amx->hea);
  amx->error = AMX_ERR_SLEEP;
  return 0;
}

static cell AMX_NATIVE_CALL n_schedule_continue(AMX *amx, cell *params) {
  if (params[0] / sizeof(*params) < 1) {
    return 0;
  }
  cell numMilliseconds = params[1];
  if (numMilliseconds < 0) {
    logprintf("[sleep] Invalid parameter: %d", numMilliseconds);
    return 0;
  }
  sleep::AddTimer(amx, numMilliseconds);
  logprintf("[sleep] Scheduled continuation at +%d ms", numMilliseconds);
  return 1;
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
  return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData) {
  logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];
  pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
  return true;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) {
  const AMX_NATIVE_INFO natives[] = {
    {"do_sleep", n_do_sleep},
    {"schedule_continue", n_schedule_continue}
  };
  int error = amx_Register(amx, natives, sizeof(natives) / sizeof(natives[0]));
  if (error != AMX_ERR_NONE) {
    logprintf("[sleep] Error: Could not register natives: %d", error);
    return error;
  }

#if 0
  int index;
  error = amx_FindPublic(amx, "sleep_callback", &index);
  if (error != AMX_ERR_NONE) {
    logprintf("[sleep] Error: sleep_callback does not exist");
    return error;
  }

  logprintf("[sleep] Executing sleep_callback");
  cell retval = 0;
  error = amx_Exec(amx, &retval, index);
  if (error != AMX_ERR_SLEEP) {
    logprintf("[sleep] Error: sleep_callback did not enter sleeep mode");
    return AMX_ERR_FORMAT;
  }
#endif

  return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) {
 return AMX_ERR_NONE;
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick() {
  sleep::ProcessTimers();
}

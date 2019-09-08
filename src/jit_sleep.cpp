#include <cassert>
#include <cstdlib>
#include <memory>
#include <vector>
#include "plugin.h"
#ifdef _WIN32
  #include <windows.h>
#else
  #include <sys/time.h>
#endif

/******************************************************************************

#include <a_samp>

native do_sleep();
native schedule_continue(numMilliseconds);

forward sleep_callback();

main() {
	print("main");
}

public sleep_callback() {
	new x = 0xc0ffee;

	schedule_continue(1000);
	#emit break
	do_sleep();
	// sleep x;
	#emit break

	printf("Hello! x = %x", x);
	print("Right?");
}

******************************************************************************/

typedef void (*logprintf_t)(const char *format, ...);

extern void *pAMXFunctions;
static logprintf_t logprintf;
static long pluginLoadTime;

namespace {
  typedef void (*TimerFunc)(AMX *amx);

  long GetTime() {
    #ifdef _WIN32
      return GetTickCount();
    #else
      timeval t;
      gettimeofday(&t, NULL);
      return t.tv_sec * t.tv_usec / 1000;
    #endif
  }

  class Timer
  {
  public:
    Timer(AMX *amx, long numMilliseconds):
      amx_(amx),
      scheduledTime_(GetTime() + numMilliseconds),
      didExecute_(false)
    {
    }

    long GetScheduledTime() const {
      return scheduledTime_;
    }

    bool DidExecute() const {
      return didExecute_;
    }

    virtual void Execute() = 0;

  protected:
    AMX *amx_;
    long scheduledTime_;
    bool didExecute_;
  };

  class ContinueTimer : public Timer
  {
  public:
    ContinueTimer(AMX *amx, long numMilliseconds):
      Timer(amx, numMilliseconds)
    {
    }

    void Execute() override {
      didExecute_ = true;
      logprintf("[sleep] Continuing execution");
      logprintf("[sleep] cip = %x", amx_->cip);
      logprintf("[sleep] pri = %x", amx_->pri);
      logprintf("[sleep] alt = %x", amx_->alt);
      logprintf("[sleep] frm = %x", amx_->frm);
      logprintf("[sleep] stk = %x", amx_->stk);
      logprintf("[sleep] hea = %x", amx_->hea);
      logprintf("[sleep] reset_stk = %x", amx_->reset_stk);
      logprintf("[sleep] reset_hea = %x", amx_->reset_hea);
      amx_Exec(amx_, NULL, AMX_EXEC_CONT);
    }
  };

  class SimpleTimer : public Timer
  {
  public:
    SimpleTimer(
      AMX *amx,
      long numMilliseconds,
      TimerFunc func
    ):
      Timer(amx, numMilliseconds),
      func_(func)
    {
      assert(func_ != NULL);
    }

    void Execute() override {
      didExecute_ = true;
      func_(amx_);
    }

  private:
    TimerFunc func_;
  };

  std::vector<Timer *> timers;

  cell *GetData(AMX *amx) {
    return amx->data != 0
      ? reinterpret_cast<cell*>(amx->data)
      : reinterpret_cast<cell*>(amx->base + ((AMX_HEADER *)amx->base)->dat);
  }

  void ExecuteSleepCallback(AMX *amx) {
    logprintf("[sleep] Executing sleep_callback");

    int index;
    int error = amx_FindPublic(amx, "sleep_callback", &index);
    if (error != AMX_ERR_NONE) {
      logprintf("[sleep] Error: sleep_callback does not exist");
      return;
    }

    cell retval = 0;
    logprintf("[sleep] before exec: frm = %x", amx->frm);
    logprintf("[sleep] before exec: stk = %x", amx->stk);
    logprintf("[sleep] before exec: hea = %x", amx->hea);
    error = amx_Exec(amx, &retval, index);
    logprintf("[sleep] after exec: frm = %x", amx->frm);
    logprintf("[sleep] after exec: stk = %x", amx->stk);
    logprintf("[sleep] after exec: hea = %x", amx->hea);
    if (error != AMX_ERR_SLEEP) {
      logprintf("[sleep] Error: sleep_callback returned %d instead of AMX_ERR_SLEEP", error);
      return;
    }
    if (amx->pri != 0xc0ffee) {
      logprintf("[sleep] Error: PRI was not saved to amx->pri");
      return;
    }
  }

  void AddContinueTimer(AMX *amx, cell numMilliseconds) {
    timers.push_back(new ContinueTimer(amx, numMilliseconds));
  }

  void AddSimpleTimer(AMX *amx, 
                      cell numMilliseconds, 
                      TimerFunc func) {
    timers.push_back(new SimpleTimer(amx, numMilliseconds, func));
  }

  void ProcessTimers() {
    long now = GetTime();
    for (std::size_t i = 0; i < timers.size(); i++) {
      Timer *timer = timers[i];
      if (!timer->DidExecute() && now >= timer->GetScheduledTime()) {
        timer->Execute();
      }
    }
  }
}

static cell AMX_NATIVE_CALL n_do_sleep(AMX *amx, cell *params) {
  logprintf("[sleep] Entering sleep!");
  logprintf("[sleep] cip = %x", amx->cip);
  logprintf("[sleep] frm = %x", amx->frm);
  logprintf("[sleep] stk = %x", amx->stk);
  logprintf("[sleep] hea = %x", amx->hea);
  logprintf("[sleep] reset_stk = %x", amx->reset_stk);
  logprintf("[sleep] reset_hea = %x", amx->reset_hea);
  amx->error = AMX_ERR_SLEEP;
  return 0xc0ffee;
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
  AddContinueTimer(amx, numMilliseconds);
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

PLUGIN_EXPORT void PLUGIN_CALL Unload() {
  for (std::vector<Timer*>::iterator it = timers.begin();
        it != timers.end(); ++it) {
    delete *it;
  }
  timers.erase(timers.begin(), timers.end());
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
  AddSimpleTimer(amx, 500, ExecuteSleepCallback);
  return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) {
 return AMX_ERR_NONE;
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick() {
  ProcessTimers();
}

// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Guard]
#ifndef _ASMJIT_CORE_LOCK_H
#define _ASMJIT_CORE_LOCK_H

// [Dependencies - AsmJit]
#include "../Core/Build.h"

// [Dependencies - Windows]
#if defined(ASMJIT_WINDOWS)
# include <windows.h>
#endif // ASMJIT_WINDOWS

// [Dependencies - Posix]
#if defined(ASMJIT_POSIX)
# include <pthread.h>
#endif // ASMJIT_POSIX

// [Api-Begin]
#include "../Core/ApiBegin.h"

namespace AsmJit {

//! @addtogroup AsmJit_Core
//! @{

// ============================================================================
// [AsmJit::Lock]
// ============================================================================

//! @brief Lock - used in thread-safe code for locking.
struct Lock
{
  // --------------------------------------------------------------------------
  // [Windows]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_WINDOWS)
  typedef CRITICAL_SECTION Handle;

  //! @brief Create a new @ref Lock instance.
  inline Lock() ASMJIT_NOTHROW { InitializeCriticalSection(&_handle); }
  //! @brief Destroy the @ref Lock instance.
  inline ~Lock() ASMJIT_NOTHROW { DeleteCriticalSection(&_handle); }

  //! @brief Lock.
  inline void lock() ASMJIT_NOTHROW { EnterCriticalSection(&_handle); }
  //! @brief Unlock.
  inline void unlock() ASMJIT_NOTHROW { LeaveCriticalSection(&_handle); }

#endif // ASMJIT_WINDOWS

  // --------------------------------------------------------------------------
  // [Posix]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_POSIX)
  typedef pthread_mutex_t Handle;

  //! @brief Create a new @ref Lock instance.
  inline Lock() ASMJIT_NOTHROW { pthread_mutex_init(&_handle, NULL); }
  //! @brief Destroy the @ref Lock instance.
  inline ~Lock() ASMJIT_NOTHROW { pthread_mutex_destroy(&_handle); }

  //! @brief Lock.
  inline void lock() ASMJIT_NOTHROW { pthread_mutex_lock(&_handle); }
  //! @brief Unlock.
  inline void unlock() ASMJIT_NOTHROW { pthread_mutex_unlock(&_handle); }
#endif // ASMJIT_POSIX

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! @brief Get handle.
  inline Handle& getHandle() ASMJIT_NOTHROW { return _handle; }
  //! @overload
  inline const Handle& getHandle() const ASMJIT_NOTHROW { return _handle; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Handle.
  Handle _handle;

  // Disable copy.
  ASMJIT_NO_COPY(Lock)
};

// ============================================================================
// [AsmJit::AutoLock]
// ============================================================================

//! @brief Scope auto locker.
struct AutoLock
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Locks @a target.
  inline AutoLock(Lock& target) ASMJIT_NOTHROW : _target(target)
  {
    _target.lock();
  }

  //! @brief Unlocks target.
  inline ~AutoLock() ASMJIT_NOTHROW
  {
    _target.unlock();
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Pointer to target (lock).
  Lock& _target;

  ASMJIT_NO_COPY(AutoLock)
};

//! @}

} // AsmJit namespace

// [Api-End]
#include "../Core/ApiEnd.h"

// [Guard]
#endif // _ASMJIT_CORE_LOCK_H

// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Guard]
#ifndef _ASMJIT_CORE_LOGGER_H
#define _ASMJIT_CORE_LOGGER_H

// [Dependencies - AsmJit]
#include "../core/build.h"
#include "../core/defs.h"
#include "../core/stringbuilder.h"

// [Dependencies - C]
#include <stdarg.h>

// [Api-Begin]
#include "../core/apibegin.h"

namespace AsmJit {

//! @addtogroup AsmJit_Logging
//! @{

// ============================================================================
// [AsmJit::Logger]
// ============================================================================

//! @brief Abstract logging class.
//!
//! This class can be inherited and reimplemented to fit into your logging
//! subsystem. When reimplementing use @c AsmJit::Logger::log() method to
//! log into your stream.
//!
//! This class also contain @c _enabled member that can be used to enable
//! or disable logging.
struct Logger
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create logger.
  ASMJIT_API Logger() ASMJIT_NOTHROW;
  //! @brief Destroy logger.
  ASMJIT_API virtual ~Logger() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Logging]
  // --------------------------------------------------------------------------

  //! @brief Abstract method to log output.
  //!
  //! Default implementation that is in @c AsmJit::Logger is to do nothing.
  //! It's virtual to fit to your logging system.
  virtual void logString(const char* buf, size_t len = kInvalidSize) ASMJIT_NOTHROW = 0;

  //! @brief Log formatter message (like sprintf) sending output to @c logString() method.
  ASMJIT_API virtual void logFormat(const char* fmt, ...) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Enabled]
  // --------------------------------------------------------------------------

  //! @brief Return @c true if logging is enabled.
  inline bool isEnabled() const ASMJIT_NOTHROW { return _enabled; }

  //! @brief Set logging to enabled or disabled.
  ASMJIT_API virtual void setEnabled(bool enabled) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Used]
  // --------------------------------------------------------------------------

  //! @brief Get whether the logger should be used.
  inline bool isUsed() const ASMJIT_NOTHROW { return _used; }

  // --------------------------------------------------------------------------
  // [LogBinary]
  // --------------------------------------------------------------------------

  //! @brief Get whether logging binary output.
  inline bool getLogBinary() const { return _logBinary; }
  //! @brief Get whether to log binary output.
  inline void setLogBinary(bool val) { _logBinary = val; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Whether logger is enabled or disabled.
  //!
  //! Default @c true.
  bool _enabled;

  //! @brief Whether logger is enabled and can be used.
  //!
  //! This value can be set by inherited classes to inform @c Logger that
  //! assigned stream (or something that can log output) is invalid. If
  //! @c _used is false it means that there is no logging output and AsmJit
  //! shouldn't use this logger (because all messages will be lost).
  //!
  //! This is designed only to optimize cases that logger exists, but its
  //! configured not to output messages. The API inside Logging and AsmJit
  //! should only check this value when needed. The API outside AsmJit should
  //! check only whether logging is @c _enabled.
  //!
  //! Default @c true.
  bool _used;

  //! @brief Whether to log instruction in binary form.
  bool _logBinary;

  ASMJIT_NO_COPY(Logger)
};

// ============================================================================
// [AsmJit::FileLogger]
// ============================================================================

//! @brief Logger that can log to standard C @c FILE* stream.
struct FileLogger : public Logger
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @c FileLogger.
  //! @param stream FILE stream where logging will be sent (can be @c NULL
  //! to disable logging).
  ASMJIT_API FileLogger(FILE* stream = NULL) ASMJIT_NOTHROW;

  //! @brief Destroy the @ref FileLogger.
  ASMJIT_API virtual ~FileLogger() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! @brief Get @c FILE* stream.
  //!
  //! @note Return value can be @c NULL.
  inline FILE* getStream() const ASMJIT_NOTHROW { return _stream; }

  //! @brief Set @c FILE* stream.
  //!
  //! @param stream @c FILE stream where to log output (can be @c NULL to
  //! disable logging).
  ASMJIT_API void setStream(FILE* stream) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Logging]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual void logString(const char* buf, size_t len = kInvalidSize) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Enabled]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual void setEnabled(bool enabled) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief C file stream.
  FILE* _stream;

  ASMJIT_NO_COPY(FileLogger)
};

// ============================================================================
// [AsmJit::StringLogger]
// ============================================================================

//! @brief String logger.
struct StringLogger : public Logger
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create new @ref StringLogger.
  ASMJIT_API StringLogger() ASMJIT_NOTHROW;

  //! @brief Destroy the @ref StringLogger.
  ASMJIT_API virtual ~StringLogger() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! @brief Get <code>char*</code> pointer which represents the serialized
  //! string.
  //!
  //! The pointer is owned by @ref StringLogger, it can't be modified or freed.
  inline const char* getString() const ASMJIT_NOTHROW { return _stringBuilder.getData(); }

  //! @brief Clear the serialized string.
  inline void clearString() ASMJIT_NOTHROW { _stringBuilder.clear(); }

  // --------------------------------------------------------------------------
  // [Logging]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual void logString(const char* buf, size_t len = kInvalidSize) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Enabled]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual void setEnabled(bool enabled) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Output.
  StringBuilder _stringBuilder;

  ASMJIT_NO_COPY(StringLogger)
};

//! @}

} // AsmJit namespace

// [Api-End]
#include "../core/apiend.h"

// [Guard]
#endif // _ASMJIT_CORE_LOGGER_H

// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

#define ASMJIT_EXPORTS

// [Dependencies - AsmJit]
#include "../core/logger.h"

// [Dependencies - C]
#include <stdarg.h>

// [Api-Begin]
#include "../core/apibegin.h"

namespace AsmJit {

// ============================================================================
// [AsmJit::Logger - Construction / Destruction]
// ============================================================================

Logger::Logger() ASMJIT_NOTHROW :
  _enabled(true),
  _used(true),
  _logBinary(false)
{
}

Logger::~Logger() ASMJIT_NOTHROW
{
}

// ============================================================================
// [AsmJit::Logger - Logging]
// ============================================================================

void Logger::logFormat(const char* fmt, ...) ASMJIT_NOTHROW
{
  char buf[1024];
  size_t len;

  va_list ap;
  va_start(ap, fmt);
  len = vsnprintf(buf, 1023, fmt, ap);
  va_end(ap);

  logString(buf, len);
}

// ============================================================================
// [AsmJit::Logger - Enabled]
// ============================================================================

void Logger::setEnabled(bool enabled) ASMJIT_NOTHROW
{
  _enabled = enabled;
  _used = enabled;
}

// ============================================================================
// [AsmJit::FileLogger - Construction / Destruction]
// ============================================================================

FileLogger::FileLogger(FILE* stream) ASMJIT_NOTHROW
  : _stream(NULL)
{
  setStream(stream);
}

FileLogger::~FileLogger() ASMJIT_NOTHROW
{
}

// ============================================================================
// [AsmJit::FileLogger - Accessors]
// ============================================================================

//! @brief Set file stream.
void FileLogger::setStream(FILE* stream) ASMJIT_NOTHROW
{
  _stream = stream;
  _used = (_enabled == true) & (_stream != NULL);
}

// ============================================================================
// [AsmJit::FileLogger - Logging]
// ============================================================================

void FileLogger::logString(const char* buf, size_t len) ASMJIT_NOTHROW
{
  if (!_used)
    return;

  if (len == kInvalidSize)
    len = strlen(buf);

  fwrite(buf, 1, len, _stream);
}

// ============================================================================
// [AsmJit::FileLogger - Enabled]
// ============================================================================

void FileLogger::setEnabled(bool enabled) ASMJIT_NOTHROW
{
  _enabled = enabled;
  _used = (_enabled == true) & (_stream != NULL);
}

// ============================================================================
// [AsmJit::StringLogger - Construction / Destruction]
// ============================================================================

StringLogger::StringLogger() ASMJIT_NOTHROW
{
}

StringLogger::~StringLogger() ASMJIT_NOTHROW
{
}

// ============================================================================
// [AsmJit::StringLogger - Logging]
// ============================================================================

void StringLogger::logString(const char* buf, size_t len) ASMJIT_NOTHROW
{
  if (!_used)
    return;
  _stringBuilder.appendString(buf, len);
}

// ============================================================================
// [AsmJit::StringLogger - Enabled]
// ============================================================================

void StringLogger::setEnabled(bool enabled) ASMJIT_NOTHROW
{
  _enabled = enabled;
  _used = enabled;
}

} // AsmJit namespace

// [Api-End]
#include "../core/apiend.h"

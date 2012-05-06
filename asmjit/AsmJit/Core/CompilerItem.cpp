// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

#define _ASMJIT_BEING_COMPILED

// [Dependencies - AsmJit]
#include "../Core/Assembler.h"
#include "../Core/Compiler.h"
#include "../Core/CompilerContext.h"
#include "../Core/CompilerFunc.h"
#include "../Core/CompilerItem.h"
#include "../Core/IntUtil.h"
#include "../Core/Logger.h"

// [Dependencies - C]
#include <stdarg.h>

// [Api-Begin]
#include "../Core/ApiBegin.h"

namespace AsmJit {

// ============================================================================
// [AsmJit::CompilerItem - Construction / Destruction]
// ============================================================================

CompilerItem::CompilerItem(Compiler* compiler, uint32_t type) ASMJIT_NOTHROW :
  _compiler(compiler),
  _prev(NULL),
  _next(NULL),
  _comment(NULL),
  _type(static_cast<uint8_t>(type)),
  _isTranslated(false),
  _isUnreachable(false),
  _reserved(0),
  _offset(kInvalidValue)
{
}

CompilerItem::~CompilerItem() ASMJIT_NOTHROW
{
}

// ============================================================================
// [AsmJit::CompilerItem - Interface]
// ============================================================================

void CompilerItem::prepare(CompilerContext& cc) ASMJIT_NOTHROW
{
  _offset = cc._currentOffset;
}

CompilerItem* CompilerItem::translate(CompilerContext& cc) ASMJIT_NOTHROW
{
  return translated();
}

void CompilerItem::emit(Assembler& a) ASMJIT_NOTHROW {}
void CompilerItem::post(Assembler& a) ASMJIT_NOTHROW {}

// ============================================================================
// [AsmJit::CompilerItem - Misc]
// ============================================================================

int CompilerItem::getMaxSize() const ASMJIT_NOTHROW
{
  // Default maximum size is -1 which means that it's not known.
  return -1;
}

bool CompilerItem::_tryUnuseVar(CompilerVar* v) ASMJIT_NOTHROW
{
  return false;
}

// ============================================================================
// [AsmJit::CompilerItem - Comment]
// ============================================================================

void CompilerItem::setComment(const char* str) ASMJIT_NOTHROW
{
  _comment = _compiler->getZoneMemory().sdup(str);
}

void CompilerItem::formatComment(const char* fmt, ...) ASMJIT_NOTHROW
{
  // The capacity should be large enough.
  char buf[80];

  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, ASMJIT_ARRAY_SIZE(buf), fmt, ap);
  va_end(ap);

  // I don't know if vsnprintf can produce non-null terminated string, in case
  // it can, we terminate it here.
  buf[ASMJIT_ARRAY_SIZE(buf) - 1] = '\0';

  setComment(buf);
}

// ============================================================================
// [AsmJit::CompilerMark - Construction / Destruction]
// ============================================================================

CompilerMark::CompilerMark(Compiler* compiler) ASMJIT_NOTHROW :
  CompilerItem(compiler, kCompilerItemMark)
{
}

CompilerMark::~CompilerMark() ASMJIT_NOTHROW
{
}

// ============================================================================
// [AsmJit::CompilerMark - Misc]
// ============================================================================

int CompilerMark::getMaxSize() const ASMJIT_NOTHROW
{
  return 0;
}

// ============================================================================
// [AsmJit::CompilerComment - Construction / Destruction]
// ============================================================================

CompilerComment::CompilerComment(Compiler* compiler, const char* str) ASMJIT_NOTHROW :
  CompilerItem(compiler, kCompilerItemComment)
{
  if (str != NULL)
    setComment(str);
}

CompilerComment::~CompilerComment() ASMJIT_NOTHROW
{
}

// ============================================================================
// [AsmJit::CompilerComment - Interface]
// ============================================================================

void CompilerComment::emit(Assembler& a) ASMJIT_NOTHROW
{
  Logger* logger = a.getLogger();

  if (logger != NULL)
    logger->logString(getComment());
}

// ============================================================================
// [AsmJit::CompilerComment - Misc]
// ============================================================================

int CompilerComment::getMaxSize() const ASMJIT_NOTHROW
{
  return 0;
}

// ============================================================================
// [AsmJit::CompilerEmbed - Construction / Destruction]
// ============================================================================

CompilerEmbed::CompilerEmbed(Compiler* compiler, const void* data, size_t length) ASMJIT_NOTHROW :
  CompilerItem(compiler, kCompilerItemEmbed)
{
  _length = length;
  memcpy(_data, data, length);
}

CompilerEmbed::~CompilerEmbed() ASMJIT_NOTHROW
{
}

// ============================================================================
// [AsmJit::CompilerEmbed - Interface]
// ============================================================================

void CompilerEmbed::emit(Assembler& a) ASMJIT_NOTHROW
{
  a.embed(_data, _length);
}

// ============================================================================
// [AsmJit::CompilerEmbed - Misc]
// ============================================================================

int CompilerEmbed::getMaxSize() const ASMJIT_NOTHROW
{
  return (int)_length;;
}

// ============================================================================
// [AsmJit::CompilerAlign - Construction / Destruction]
// ============================================================================

CompilerAlign::CompilerAlign(Compiler* compiler, uint32_t size) ASMJIT_NOTHROW :
  CompilerItem(compiler, kCompilerItemAlign), _size(size)
{
}

CompilerAlign::~CompilerAlign() ASMJIT_NOTHROW
{
}

// ============================================================================
// [AsmJit::CompilerAlign - Misc]
// ============================================================================

int CompilerAlign::getMaxSize() const ASMJIT_NOTHROW
{
  if (_size == 0)
    return 0;
  else
    return static_cast<int>(_size - 1);
}

// ============================================================================
// [AsmJit::CompilerHint - Construction / Destruction]
// ============================================================================

CompilerHint::CompilerHint(Compiler* compiler, CompilerVar* var, uint32_t hintId, uint32_t hintValue) ASMJIT_NOTHROW :
  CompilerItem(compiler, kCompilerItemHint),
  _var(var),
  _hintId(hintId),
  _hintValue(hintValue)
{
  ASMJIT_ASSERT(var != NULL);
}

CompilerHint::~CompilerHint() ASMJIT_NOTHROW
{
}

// ============================================================================
// [AsmJit::CompilerTarget - Construction / Destruction]
// ============================================================================

CompilerTarget::CompilerTarget(Compiler* compiler, const Label& label) ASMJIT_NOTHROW :
  CompilerItem(compiler, kCompilerItemTarget),
  _label(label),
  _from(NULL),
  _state(NULL),
  _jumpsCount(0)
{
}

CompilerTarget::~CompilerTarget() ASMJIT_NOTHROW
{
}

// ============================================================================
// [AsmJit::CompilerTarget - Misc]
// ============================================================================

int CompilerTarget::getMaxSize() const ASMJIT_NOTHROW
{
  return 0;
}

// ============================================================================
// [AsmJit::CompilerInst - Construction / Destruction]
// ============================================================================

CompilerInst::CompilerInst(Compiler* compiler, uint32_t code, Operand* opData, uint32_t opCount) ASMJIT_NOTHROW :
  CompilerItem(compiler, kCompilerItemInst),
  _code(code),
  _emitOptions(static_cast<uint8_t>(compiler->_emitOptions)),
  _instFlags(0),
  _operandsCount(static_cast<uint8_t>(opCount)),
  _variablesCount(0),
  _operands(opData)
{
  // Each created instruction takes emit options and clears it.
  compiler->_emitOptions = 0;
}

CompilerInst::~CompilerInst() ASMJIT_NOTHROW
{
}

// ============================================================================
// [AsmJit::CompilerInst - GetJumpTarget]
// ============================================================================

CompilerTarget* CompilerInst::getJumpTarget() const ASMJIT_NOTHROW
{
  return NULL;
}

} // AsmJit namespace

// [Api-Begin]
#include "../Core/ApiBegin.h"

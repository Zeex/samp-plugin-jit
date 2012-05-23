// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

#define _ASMJIT_BEING_COMPILED

// [Dependencies - AsmJit]
#include "../Core/Defs.h"
#include "../Core/IntUtil.h"
#include "../Core/ZoneMemory.h"

// [Api-Begin]
#include "../Core/ApiBegin.h"

namespace AsmJit {

// ============================================================================
// [AsmJit::ZoneMemory]
// ============================================================================

ZoneMemory::ZoneMemory(size_t chunkSize) ASMJIT_NOTHROW
{
  _chunks = NULL;
  _total = 0;
  _chunkSize = chunkSize;
}

ZoneMemory::~ZoneMemory() ASMJIT_NOTHROW
{
  reset();
}

void* ZoneMemory::alloc(size_t size) ASMJIT_NOTHROW
{
  ZoneChunk* cur = _chunks;

  // Align to 4 or 8 bytes.
  size = IntUtil::align<size_t>(size, sizeof(size_t));

  if (cur == NULL || cur->getRemainingBytes() < size)
  {
    size_t chSize = _chunkSize;
 
    if (chSize < size)
      chSize = size;

    cur = (ZoneChunk*)ASMJIT_MALLOC(sizeof(ZoneChunk) - sizeof(void*) + chSize);
    if (cur == NULL)
      return NULL;

    cur->prev = _chunks;
    cur->pos = 0;
    cur->size = _chunkSize;
    _chunks = cur;
  }

  uint8_t* p = cur->data + cur->pos;
  cur->pos += size;
  _total += size;
  return (void*)p;
}

char* ZoneMemory::sdup(const char* str) ASMJIT_NOTHROW
{
  if (str == NULL) return NULL;

  size_t len = strlen(str);
  if (len == 0) return NULL;

  // Include NULL terminator and limit string length.
  if (++len > 256)
    len = 256;

  char* m = reinterpret_cast<char*>(alloc(IntUtil::align<size_t>(len, 16)));
  if (m == NULL)
    return NULL;

  memcpy(m, str, len);
  m[len - 1] = '\0';
  return m;
}

void ZoneMemory::clear() ASMJIT_NOTHROW
{
  ZoneChunk* cur = _chunks;

  if (cur == NULL)
    return;

  _chunks->pos = 0;
  _chunks->prev = NULL;
  _total = 0;

  cur = cur->prev;
  while (cur != NULL)
  {
    ZoneChunk* prev = cur->prev;
    ASMJIT_FREE(cur);
    cur = prev;
  }
}

void ZoneMemory::reset() ASMJIT_NOTHROW
{
  ZoneChunk* cur = _chunks;

  _chunks = NULL;
  _total = 0;

  while (cur != NULL)
  {
    ZoneChunk* prev = cur->prev;
    ASMJIT_FREE(cur);
    cur = prev;
  }
}

} // AsmJit namespace

// [Api-End]
#include "../Core/ApiEnd.h"

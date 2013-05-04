// Copyright (c) 2012-2013 Zeex
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef AMXJIT_AMXPTR_H
#define AMXJIT_AMXPTR_H

#include <cassert>
#include <cstddef>
#include <amx/amx.h>

namespace amxjit {

// AMXPtr is a lightweight wrapper around the AMX structure with a number
// of useful methods. Instances of this class can be passed by value without
// a harm just like raw AMX pointers and are in fact implicitly converted to
// and from AMX* when possible.
class AMXPtr {
 public:
  AMXPtr() : amx(0) {}
  AMXPtr(AMX *amx) : amx(amx) {}

  operator AMX*() { return amx; }
  operator AMX*() const { return amx; }

  AMX *operator->() { return AccessAmx(); }
  const AMX *operator->() const { return AccessAmx(); }

  AMX *GetStruct() const {
    return amx;
  }

  AMX_HEADER *GetHeader() const {
    return reinterpret_cast<AMX_HEADER*>(AccessAmx()->base);
  }

  unsigned char *GetCode() const {
    return AccessAmx()->base + GetHeader()->cod;
  }

  std::size_t GetCodeSize() const {
    return GetHeader()->dat - GetHeader()->cod;
  }

  unsigned char *GetData() const {
    return AccessAmx()->data != 0 ? AccessAmx()->data
                                  : AccessAmx()->base + GetHeader()->dat;
  }
  std::size_t GetDataSize() const {
    return GetHeader()->hea - GetHeader()->dat;
  }

  int GetNumPublics() const {
    return (GetHeader()->natives - GetHeader()->publics)
            / GetHeader()->defsize;
  }

  int GetNumNatives() const {
    return (GetHeader()->libraries - GetHeader()->natives)
            / GetHeader()->defsize;
  }

  AMX_FUNCSTUBNT *GetPublics() const {
    return reinterpret_cast<AMX_FUNCSTUBNT*>(GetHeader()->publics
                                             + AccessAmx()->base);
  }

  AMX_FUNCSTUBNT *GetNatives() const {
    return reinterpret_cast<AMX_FUNCSTUBNT*>(GetHeader()->natives
                                             + AccessAmx()->base);
  }

  cell GetPublicAddress(cell index) const;
  cell GetNativeAddress(cell index) const;

  const char *GetPublicName(cell index) const;
  const char *GetNativeName(cell index) const;

  cell FindPublic(cell address) const;
  cell FindNative(cell address) const;

  cell *GetStack() const {
    return reinterpret_cast<cell*>(GetData() + AccessAmx()->stk);
  }

  cell GetStackSize() const {
    return GetHeader()->stp - GetHeader()->hea;
  }

  cell *PushStack(cell value);

  cell PopStack();
  void PopStack(int ncells);

 private:
  AMX *AccessAmx() {
    assert(amx != 0);
    return amx;
  }

  const AMX *AccessAmx() const {
    assert(amx != 0);
    return amx;
  }

 private:
  AMX *amx;
};

} // namespace amxjit

#endif // !AMXJIT_AMXPTR_H

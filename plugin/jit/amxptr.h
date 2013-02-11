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

#ifndef JIT_AMXPTR_H
#define JIT_AMXPTR_H

#include <cstddef>
#include <amx/amx.h>

namespace jit {

// AMXPtr is a lightweight wrapper around the AMX structure with a number
// of useful methods. Instances of this class can be passed by value without
// a harm just like raw AMX pointers and are in fact implicitly converted to
// and from AMX* when possible.
class AMXPtr {
 public:
  AMXPtr(AMX *amx);

  operator AMX*() { return amx_; }
  operator AMX*() const { return amx_; }

  AMX *operator->() { return amx_; }
  AMX *operator->() const { return amx_; }

  AMX *amx() const {
    return amx_;
  }
  AMX_HEADER *hdr() const {
    return reinterpret_cast<AMX_HEADER*>(amx()->base);
  }

  unsigned char *code() const {
    return amx()->base + hdr()->cod;
  }
  std::size_t code_size() const {
    return hdr()->dat - hdr()->cod;
  }

  unsigned char *data() const {
    return amx()->data != 0 ? amx()->data : amx()->base + hdr()->dat;
  }
  std::size_t data_size() const {
    return hdr()->hea - hdr()->dat;
  }

  int num_publics() const {
    return (hdr()->natives - hdr()->publics) / hdr()->defsize;
  }
  int num_natives() const {
    return (hdr()->libraries - hdr()->natives) / hdr()->defsize;
  }

  AMX_FUNCSTUBNT *publics() const {
    return reinterpret_cast<AMX_FUNCSTUBNT*>(hdr()->publics + amx()->base);
  }
  AMX_FUNCSTUBNT *natives() const {
    return reinterpret_cast<AMX_FUNCSTUBNT*>(hdr()->natives + amx()->base);
  }

  cell get_public_addr(cell index) const;
  cell get_native_addr(cell index) const;

  const char *get_public_name(cell index) const;
  const char *get_native_name(cell index) const;

  cell find_public(cell address) const;
  cell find_native(cell address) const;

  cell *stack() const {
    return reinterpret_cast<cell*>(data() + amx()->stk);
  }
  cell stack_size() const {
    return hdr()->stp - hdr()->hea;
  }

  cell *push(cell value);
  cell pop();
  void pop(int ncells);

private:
  AMX *amx_;
};

} // namespace jit

#endif // !JIT_AMXPTR_H

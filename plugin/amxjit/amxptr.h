// Copyright (c) 2012-2014 Zeex
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

#include <cstddef>
#include <amx/amx.h>

namespace amxjit {

class AMXPtr {
 public:
  AMXPtr(): amx_(0) {}
  AMXPtr(AMX *amx): amx_(amx) {}

  operator AMX*() { return amx_; }
  operator AMX*() const { return amx_; }

  AMX *operator->() { return AccessAmx(); }
  const AMX *operator->() const { return AccessAmx(); }

  AMX *raw() const { return amx_; }
  AMX_HEADER *header() const;

  unsigned char *code() const;
  unsigned char *data() const;

  std::size_t code_size() const;
  std::size_t data_size() const ;

  int num_publics() const;
  int num_natives() const;

  AMX_FUNCSTUBNT *publics() const;
  AMX_FUNCSTUBNT *natives() const;

  cell GetPublicAddress(cell index) const;
  cell GetNativeAddress(cell index) const;

  const char *GetPublicName(cell index) const;
  const char *GetNativeName(cell index) const;

  cell FindPublic(cell address) const;
  cell FindNative(cell address) const;

 private:
  AMX *AccessAmx();
  const AMX *AccessAmx() const;

 private:
  AMX *amx_;
};

} // namespace amxjit

#endif // !AMXJIT_AMXPTR_H

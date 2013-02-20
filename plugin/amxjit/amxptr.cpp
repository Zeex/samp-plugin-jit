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

#include "amxptr.h"

namespace amxjit {

AMXPtr::AMXPtr(AMX *amx)
  : amx_(amx)
{
}

cell AMXPtr::get_public_addr(cell index) const {
  if (index == AMX_EXEC_MAIN) {
    return hdr()->cip;
  }
  if (index < 0 || index >= num_publics()) {
    return 0;
  }
  return publics()[index].address;
}

cell AMXPtr::get_native_addr(cell index) const {
  if (index < 0 || index >= num_natives()) {
    return 0;
  }
  return natives()[index].address;
}

cell AMXPtr::find_public(cell address) const {
  for (int i = 0; i < num_publics(); i++) {
    if (publics()[i].address == static_cast<ucell>(address)) {
      return i;
    }
  }
  return -1;
}

cell AMXPtr::find_native(cell address) const {
  for (int i = 0; i < num_natives(); i++) {
    if (natives()[i].address == static_cast<ucell>(address)) {
      return i;
    }
  }
  return -1;
}

const char *AMXPtr::get_public_name(cell index) const {
  if (index < 0 || index >= num_publics()) {
    return 0;
  }
  return reinterpret_cast<char*>(amx_->base + publics()[index].nameofs);
}

const char *AMXPtr::get_native_name(cell index) const {
  if (index < 0 || index >= num_natives()) {
    return 0;
  }
  return reinterpret_cast<char*>(amx_->base + natives()[index].nameofs);
}

cell *AMXPtr::push(cell value) {
  amx_->stk -= sizeof(cell);
  cell *s = stack();
  *s = value;
  return s;
}

cell AMXPtr::pop() {
  cell *s = stack();
  amx_->stk += sizeof(cell);
  return *s;
}

void AMXPtr::pop(int ncells) {
  amx_->stk += ncells * sizeof(cell);
}

} // namespace amxjit

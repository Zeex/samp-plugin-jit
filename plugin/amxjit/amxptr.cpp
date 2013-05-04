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

cell AMXPtr::GetPublicAddress(cell index) const {
  if (index == AMX_EXEC_MAIN) {
    AMX_HEADER *header = GetHeader();
    return header->cip;
  }
  if (index >= 0 || index < GetNumPublics()) {
    AMX_FUNCSTUBNT *publics = GetPublics();
    return publics[index].address;
  }
  return 0;
}

cell AMXPtr::GetNativeAddress(cell index) const {
  if (index >= 0 && index < GetNumNatives()) {
    AMX_FUNCSTUBNT *natives = GetNatives();
    return natives[index].address;
  }
  return 0;
}

cell AMXPtr::FindPublic(cell address) const {
  int numPublics = GetNumPublics();
  AMX_FUNCSTUBNT *publics = GetPublics();
  for (int i = 0; i < numPublics; i++) {
    if (publics[i].address == static_cast<ucell>(address)) {
      return i;
    }
  }
  return -1;
}

cell AMXPtr::FindNative(cell address) const {
  int numNatives = GetNumNatives();
  AMX_FUNCSTUBNT *natives = GetNatives();
  for (int i = 0; i < numNatives; i++) {
    if (natives[i].address == static_cast<ucell>(address)) {
      return i;
    }
  }
  return -1;
}

const char *AMXPtr::GetPublicName(cell index) const {
  if (index >= 0 && index < GetNumPublics()) {
    return reinterpret_cast<char*>(AccessAmx()->base
                                   + GetPublics()[index].nameofs);
  }
  return 0;
}

const char *AMXPtr::GetNativeName(cell index) const {
  if (index >= 0 && index < GetNumNatives()) {
    return reinterpret_cast<char*>(AccessAmx()->base
                                   + GetNatives()[index].nameofs);
  }
  return 0;
}

cell *AMXPtr::PushStack(cell value) {
  AccessAmx()->stk -= sizeof(cell);
  cell *s = GetStack();
  *s = value;
  return s;
}

cell AMXPtr::PopStack() {
  cell *s = GetStack();
  AccessAmx()->stk += sizeof(cell);
  return *s;
}

void AMXPtr::PopStack(int ncells) {
  AccessAmx()->stk += ncells * sizeof(cell);
}

} // namespace amxjit

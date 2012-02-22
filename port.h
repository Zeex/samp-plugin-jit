// Copyright (c) 2012, Sergey Zolotarev
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met: 
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer. 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution. 
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// // LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef SAMPJIT_PORT_H
#define SAMPJIT_PORT_H

#if defined LINUX
	#include <sys/mman.h>
#else
	#include <Windows.h> 
#endif

#if defined _MSC_VER
	#if !defined CDECL
		#define CDECL __cdecl
	#endif
	#if !defined STDCALL
		#define STDCALL __stdcall
	#endif
#elif defined __GNUC__
	#if !defined CDECL
		#define CDECL __attribute__((cdecl))
	#endif
	#if !defined STDCALL
		#define STDCALL __attribte((stdcall));
	#endif
#endif

#if defined WIN32

	#define PAGE_ALIGN(x) (x)

	#define PROT_NONE  0x0 
	#define PROT_READ  0x1 
	#define PROT_WRITE 0x2 
	#define PROT_EXEC  0x4 

	static int mprotect(void *addr, size_t len, int prot) {
		DWORD new_prot;
		DWORD old_prot;

		if (prot == PROT_NONE) {
			new_prot = PAGE_NOACCESS;
		} else if (prot == PROT_READ) {
			new_prot = PAGE_READONLY;
		} else if ((prot & PROT_WRITE) && (prot & PROT_READ)) {
			new_prot = PAGE_EXECUTE_READWRITE;
		} else if ((prot & PROT_READ)) {
			new_prot = PAGE_EXECUTE_READ;
		} else if ((prot & PROT_EXEC)) {
			new_prot = PAGE_EXECUTE;
		}

		return !VirtualProtect((LPVOID)addr, (SIZE_T)len, new_prot, &old_prot);
	}

#else

	#define PAGE_ALIGN(x) (void*)(((int)x + sysconf(_SC_PAGESIZE) - 1) & ~(sysconf(_SC_PAGESIZE) - 1))

#endif

#endif // !SAMPJIT_PORT_H

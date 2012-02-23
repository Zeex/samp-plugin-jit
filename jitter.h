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

#ifndef SAMPJIT_JITTER_H
#define SAMPJIT_JITTER_H

#include <iosfwd>
#include <list>
#include <map>
#include <string>

#include "amx.h"
#include "jitasm.h"

namespace sampjit {

class Jitter;

// JitFunction represents a JIT-compiled AMX function.
class JitFunction : public jitasm::function<void, JitFunction> {
public:
	JitFunction(Jitter *jitter, ucell address);

	void main();

	void SetLabel(cell address, const std::string &tag = std::string());
	std::string GetLabelName(cell address, const std::string &tag = std::string()) const;

private:
	// Disable copying.
	JitFunction(const JitFunction &);
	JitFunction &operator=(const JitFunction &);

	Jitter *jitter_;
	ucell   address_;	
};

class Jitter {
	friend class JitFunction;
public:
	typedef cell (CDECL *PublicFunction)();

	Jitter(AMX *amx);
	~Jitter();

	inline AMX        *GetAmx()       { return amx_; }
	inline AMX_HEADER *GetAmxHeader() { return amxhdr_; }

	// Get pointer to start of AMX data section.
	inline unsigned char *GetAmxData() { return data_; }

	// Get pointer to start of AMX code section.
	inline unsigned char *GetAmxCode() { return code_; }

	// Get assembled function (and assemble if needed).
	JitFunction *GetFunction(ucell address);

	// Assemble AMX function at the specified address.
	JitFunction *AssembleFunction(ucell address);

	// Call a public function (and assemble if needed).
	// In contrast to CallFunction(), this method also copies the arguments
	// pushed to the AMX stack with amx_Push() and friends to the physical
	// stack (including paramcount) prior to the call.
	// The index must be a valid non-negative index of the publics table
	// and -1 for main().
	int CallPublicFunction(int index, cell *retval);

	// Output generated code to a stream.
	// The code includes only those functions that were actually called.
	void DumpCode(std::ostream &stream) const;

private:
	// Disable copying.
	Jitter(const Jitter &);
	Jitter &operator=(const Jitter &);

private:
	AMX        *amx_;
	AMX_HEADER *amxhdr_;

	unsigned char *data_;
	unsigned char *code_;

	// proc_map_ maps AMX funtions to their JIT equivalents.
	typedef std::map<ucell, JitFunction*> ProcMap;
	ProcMap proc_map_;
};

} // namespace sampjit

#endif // !SAMPJIT_JITTER_H

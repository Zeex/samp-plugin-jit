// Copyright (C) 2013-2015 Zeex
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#if defined JIT_INC
	#endinput
#endif
#define JIT_INC

stock const JIT_ASM_JUMP_CAPABLE = 1;
stock const JIT_COMPLETION_CAPABLE = 2;

static stock
	bool:gs_JIT_Present = false,
	bool:gs_JIT_Enabled = false,
	bool:gs_JIT_Complete = false,
	bool:gs_JIT_Failed = false,
	gs_JIT_Capabilities = 0;

forward OnJITCapabilities(capablities);

public OnJITCapabilities(capablities) {
	gs_JIT_Capabilities = capablities;
	#if defined JIT_OnJITCapabilities
		JIT_OnJITCapabilities(capablities);
	#endif
}

#if defined JIT_OnJITCapabilities
	forward JIT_OnJITCapabilities(capablities);
#endif
#if defined _ALS_OnJITCapabilities
	#error OnJITCapabilities hooked before including <jit>
#endif
#define OnJITCapabilities JIT_OnJITCapabilities
#define _ALS_OnJITCapabilities

forward OnJITCompile();

public OnJITCompile() {
	gs_JIT_Present = true;
	#if defined JIT_OnJITCompile
		if (!JIT_OnJITCompile()) {
			return 0;
		}
	#endif
	gs_JIT_Enabled = true;
	return 1;
}

#if defined JIT_OnJITCompile
	forward JIT_OnJITCompile();
#endif
#if defined _ALS_OnJITCompile
	#error OnJITCompile hooked before including <jit>
#endif
#define OnJITCompile JIT_OnJITCompile
#define _ALS_OnJITCompile

forward OnJITComplete(success);

public OnJITComplete(success) {
	gs_JIT_Complete = true;
	gs_JIT_Failed = !success;
	#if defined JIT_OnJITComplete
		JIT_OnJITComplete(success);
	#endif
}

#if defined JIT_OnJITComplete
	forward JIT_OnJITComplete(success);
#endif
#if defined _ALS_OnJITComplete
	#error OnJITComplete hooked before including <jit>
#endif
#define OnJITComplete JIT_OnJITComplete
#define _ALS_OnJITComplete

stock bool:IsJITPresent() {
	// Using "#emit LCTRL 7" doesn't work in "OnJITCompile", which is the time
	// most likely to require knowledge of the JIT existing.
	return gs_JIT_Present;
}

stock bool:IsJITComplete() {
	return gs_JIT_Complete;
}

stock bool:IsJITFailed() {
	return gs_JIT_Failed;
}

stock bool:IsJITEnabled() {
	return gs_JIT_Enabled;
}

stock bool:IsJITASMJumpCapable() {
	return !!(gs_JIT_Capabilities & JIT_ASM_JUMP_CAPABLE);
}

stock bool:IsJITCompletionCapable() {
	return !!(gs_JIT_Capabilities & JIT_COMPLETION_CAPABLE);
}


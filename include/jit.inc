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

//
// This variable will be set to true if the JIT plugin is loaded.
//
public const bool:__JIT = false;

//
// OnJITCompile() is called when the script is about to be compiled by
// the JIT plugin
//
forward OnJITCompile();

//
// OnJITError() is called when an error occurs during JIT compilation.
// See your server log for additional error information.
//
forward OnJITError();

//
// Checks if the script is currently running in JIT mode. This function
// will return false in OnJITCompile().
//
stock bool:IsJITPresent() {
	#emit zero.pri
	#emit lctrl 7
	#emit retn
	return false; // make compiler happy
}

//
// Checks if the JIT plugin supports the fast jumps feature that was added
// in version 2.1.
//
// https://github.com/Zeex/samp-plugin-jit/issues/51
//
stock bool:IsJITASMJumpCapable() {
	#emit lctrl 6
	#emit move.alt
	#emit lctrl 8
	#emit neq
	#emit retn
	return false; // make compiler happy
}


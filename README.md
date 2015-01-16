[JIT plugin][github]
====================

[![Version][version_badge]][version]
[![Build Status][build_status]][build]

This is a Just-in-Time (JIT) compiler for AMX 3.x and a plugin for the SA-MP
server. It compiles AMX bytecode that is produced by the Pawn compiler into
native x86 code at runtime for increased performance.

Download
--------

Get latest binaries for Windows and Linux [here][download].

Limitations
-----------

Everything that doesn't use `#emit` will probably work okay. Otherwise, it
depends on the code. There are some advanced `#emit` hacks that simply won't
work with this JIT. Self-modifying code is one example (although in some
caes it's possible to fix, see [Detecting JIT at runtime][wiki-detecting]).

If you're using [YSI][ysi] this plugin most likely will not work for you and
simply crash your server.

How it works
------------

It's a pretty simple JIT.

Code is compiled once and stored in memory for the lifetime of the server,
or until the script gets unloaded in case of filterscripts. Thus there may
be a small delay during the server startup.

Most of the compilation process is a mere translation of AMX opcodes into
seuquences of corresponding machine instructions using essentially a giant
`switch` statement. There are some places though where the compiler tries
to be a little bit smarter: for instance, it will replace calls to common
floating-point functions (those found in float.inc) with equivalent code
using the x87 FPU instruction set.

Native code generation is done via [AsmJit][asmjit], a wonderful assembler
library for x86/x86-64 :+1:. There also was an attempt to use LLVM as an
alternative backend but it was quickly abandoned...

License
-------

Licensed under the 2-clause BSD license. See the LICENSE.txt file.

[github]: https://github.com/Zeex/samp-plugin-jit
[version]: http://badge.fury.io/gh/Zeex%2Fsamp-plugin-jit
[version_badge]: https://badge.fury.io/gh/Zeex%2Fsamp-plugin-jit.svg
[build]: https://travis-ci.org/Zeex/samp-plugin-jit
[build_status]: https://travis-ci.org/Zeex/samp-plugin-jit.svg?branch=master
[v8]: https://code.google.com/p/v8/
[asmjit]: https://github.com/kobalicek/asmjit
[wiki-detecting]: https://github.com/Zeex/samp-plugin-jit/wiki/Detecting-JIT-at-runtime
[ysi]: https://github.com/Y-Less/YSI
[download]: https://github.com/Zeex/samp-plugin-jit/releases

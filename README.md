JIT plugin for SA-MP server
===========================

https://github.com/Zeex/samp-jit-plugin


Introduction
------------

This is a Just-In-Time compiler for AMX version 3.2 (and probably older/newer) 
and a [SA-MP](http://www.sa-mp.com) server plugin that works on both Linux and
Windows.


Installation
------------

Download a zip or tar.gz from [here](https://github.com/Zeex/samp-jit-plugin/downloads),
unpack it and:

*	Windows

	Copy `jit.dll` to the `plugins` folder inside your server's root (create it
	if it doesn't exist) and add `jit` to the list of plugins in `server.cfg`.

*	Linux

	Copy `jit.so` to the `plugins` folder inside your server's root (create it
	if it doesn't exist) and add `jit.so` to the list of plugins in `server.cfg`.
	

Configuration
-------------

There are a few options you can change via `server.cfg`:

*	`jit_dump_asm 0/1`

	Dumps a nicely-formatted assembly listing of the JIT generated code to `path/to/amx.asm`,
	for example `gamemodes/lvdm.amx.asm`.

*	`jit_dump_bin 0/1`

	Dumps raw (binary) code to `path/to/amx.bin`. This can be useful if you have something
	like [IDA](http://www.hex-rays.com/products/ida/index.shtml).


Bug tracker
-----------

https://github.com/Zeex/samp-jit-plugin/issues

SA-MP JIT plugin
----------------

This is a JIT plugin for SA-MP server. It translates AMX bytecode (the code 
produced by Pawn compiler) to native x86 code at run time to speed up script 
execution.

New server.cfg vars:

  * jit_stack <size>

    Specifies how much memory must be allocated for JIT stack, in bytes.
    Default stack size is 1 MB.

  * jit_listing <0|1>

    Output assembly listing to a text file named like so: 
      
    <amx_path>/<amx_name>.asm

    For example, if you run LVDM JIT will output its code to:

    gamemodes/lvdm.asm

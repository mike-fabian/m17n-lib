set main

# br mdebug_hook

source .gdb.util

set env LD_LIBRARY_PATH=../src/.libs:./.libs:/usr/local/lib

set env EF_ALLOW_MALLOC_0=1
set env EF_PROTECT_FREE=1
set env EF_OVERRUN_DETECTION=1
set env EF_FREE_WIPES=1

set env MDEBUG_INIT=1
set env MDEBUG_FINI=1
set env MDEBUG_CHARSET=1
set env MDEBUG_CODING=1
set env MDEBUG_DATABASE=1
set env MDEBUG_FONT=1
set env MDEBUG_FONT_FLT=0
set env MDEBUG_FONT_OTF=1

set env PURIFYOPTIONS=-chain-length="20"

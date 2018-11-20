/*

Module:  lmic.h

Function:
        Deprecated C++ top-level include file (use <arduino_lmic.h> instead).

Copyright & License:
        See accompanying LICENSE file.

Author:
        Terry Moore, MCCI       November 2018

Note:
        This header file is deprecated and is included for
        transitional purposes. It's deprecated because it's
        confusing to have src/lmic.h (this file) and src/lmic/lmic.h
        (the API file for the C library). We can't take it out
        yet, because it would inconvenience the world, but
        we can hope that someday it will wither away (on a major
        version bump).

        Please don't add any new functionality in this file;
        it is just a wrapper for arduino_lmic.h.

*/

#include "arduino_lmic.h"

/* end of file */

# This is the framework I extrace for my no-engine C++ game


# How to run it
Builds the native executable (main) with ANGLE and Metal backend.

    make -f Makefile.mac main

Build the main app with support for hot-reloading game logic.

    make -f Makefile.mac main HOT_RELOAD=1

Builds only game.so, the dynamic library containing reloadable game logic.

    make -f Makefile.mac game

Builds Emscripted web export

    make -f Makefile.emscripten main 

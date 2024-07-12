#!/bin/bash

# -G <generator-name>          = Specify a build system generator.
# -S <path-to-source>          = Explicitly specify a source directory.
# -C <initial-cache>           = Pre-load a script to populate the cache.
# -B <path-to-build>           = Explicitly specify a build directory.
cmake                                                                                    \
    -G "Unix Makefiles"                                                                  \
    -S src                                                                               \
    -C scripts/1.cfg.cmake.x64-linux.txt                                                 \
    -B build/x64-linux                                                                   \
    -Wno-dev                                                                             \
    -DOPENSSL_ROOT_DIR="/mnt/d/__develop__/vcpkg/installed/x64-linux"                    \
    -DOPENSSL_LIBRARIES="/mnt/d/__develop__/vcpkg/installed/x64-linux/lib"

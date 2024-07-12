@echo off

@REM -G <generator-name>          = Specify a build system generator.
@REM -S <path-to-source>          = Explicitly specify a source directory.
@REM -C <initial-cache>           = Pre-load a script to populate the cache.
@REM -B <path-to-build>           = Explicitly specify a build directory.
cmake                                                                                                        ^
    -S src                                                                                                   ^
    -C scripts/1.cfg.cmake.x64-windows-static.txt                                                            ^
    -B build/x64-windows-static                                                                              ^
    -A x64                                                                                                   ^
    -Wno-dev                                                                                                 ^
    -DOPENSSL_ROOT_DIR="d:/__develop__/vcpkg/installed/x64-windows-static"                                   ^
    -DOPENSSL_LIBRARIES="d:/__develop__/vcpkg/installed/x64-windows-static/lib"

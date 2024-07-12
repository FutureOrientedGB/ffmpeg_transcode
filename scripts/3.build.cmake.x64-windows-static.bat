@echo off

cmake                                                                          ^
    --build build/x64-windows-static                                           ^
    --target ALL_BUILD                                                         ^
    --config Release

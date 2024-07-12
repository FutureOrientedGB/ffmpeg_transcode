@echo off


@REM install cmake >= 3.29.6 and add it's bin path to system environment PATH


@REM install visual studio >= 2022 with visual c++ desktop development tools


@REM cd to project parent dir
cd ..

@REM install vcpkg
@REM commit f7423ee180c4b7f40d43402c2feb3859161ef625 (HEAD -> master, tag: 2024.06.15)
@REM Author: MonicaLiu <110024546+MonicaLiu0311@users.noreply.github.com>
@REM Date:   Fri Jun 14 11:51:31 2024 -0700
@REM 
@REM     [caf] Update to 0.19.6 (#39288)
@REM 
@REM  ports/caf/fix_cxx17.patch | 31 ++++++++++++-------------------
@REM  ports/caf/portfile.cmake  |  4 ++--
@REM  ports/caf/vcpkg.json      |  2 +-
@REM  versions/baseline.json    |  2 +-
@REM  versions/c-/caf.json      |  5 +++++
@REM  5 files changed, 21 insertions(+), 23 deletions(-)
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
git reset --hard f7423ee180c4b7f40d43402c2feb3859161ef625

@REM bootstrap vcpkg
bootstrap-vcpkg.bat

@REM vcpkg build and install libraries
vcpkg install pkgconf
vcpkg install spdlog
vcpkg install fmt
vcpkg install cli11
vcpkg install thrift
vcpkg install thrift
vcpkg install grpc[codegen]
vcpkg install qtbase

@REM return project dir
cd ..\..


#!/bin/bash

# apt
sudo apt install linux-libc-dev ntpdate

# update datetime (for ./vcpkg install pkgconf to fix ERROR: Clock skew detected. File vcpkg/buildtrees/pkgconf/x64-linux-dbg/meson-private/coredata.dat has a time stamp 0.4692s in the future)

# install latest python through minconda (for ./vcpkg install pkgconf)
wget https://mirrors.tuna.tsinghua.edu.cn/anaconda/miniconda/Miniconda3-py39_24.4.0-0-Linux-x86_64.sh
bash Miniconda3-py39_24.4.0-0-Linux-x86_64.sh
# press ENTER to view licenses
# press yes when "'Please answer 'yes' or 'no':" displayed on screen
# press ENTER to install into default location when "Miniconda3 will now be installed into this location:" displayed on screen
# press yes when "You can undo this by running `conda init --reverse $SHELL`? [yes|no]" displayed on screen
source ~/.bashrc

# install cmake >= 3.29.6 and add it's bin path to system environment PATH
cd ~
mkdir .local
cd .local
wget https://github.com/Kitware/CMake/releases/download/v3.29.6/cmake-3.29.6-linux-x86_64.sh
bash cmake-3.29.6-linux-x86_64.sh
# press y to accept licenses
# press no to install cmake resources to ~/.local
echo $PATH = $PATH:~/.local/bin >> ~/.bashrc
cd -


# install llvm and set soft link for clang and clang++
wget https://apt.llvm.org/llvm.sh
sudo bash llvm 18
cd /usr/bin
sudo ln -s clang++-18 clang++
sudo ln -s clang-18 clang
cd -


# cd to project parent dir
cd ..


# install vcpkg
# commit f7423ee180c4b7f40d43402c2feb3859161ef625 (HEAD -> master, tag: 2024.06.15)
# Author: MonicaLiu <110024546+MonicaLiu0311@users.noreply.github.com>
# Date:   Fri Jun 14 11:51:31 2024 -0700
# 
#     [caf] Update to 0.19.6 (#39288)
# 
#  ports/caf/fix_cxx17.patch | 31 ++++++++++++-------------------
#  ports/caf/portfile.cmake  |  4 ++--
#  ports/caf/vcpkg.json      |  2 +-
#  versions/baseline.json    |  2 +-
#  versions/c-/caf.json      |  5 +++++
#  5 files changed, 21 insertions(+), 23 deletions(-)
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
git reset --hard f7423ee180c4b7f40d43402c2feb3859161ef625

# bootstrap vcpkg
./bootstrap-vcpkg.sh

# vcpkg build and install libraries
./vcpkg install pkgconf
./vcpkg install spdlog
./vcpkg install fmt
./vcpkg install cli11
./vcpkg install thrift
./vcpkg install thrift
./vcpkg install grpc[codegen]
./vcpkg install qtbase


@REM return project dir
cd ../..

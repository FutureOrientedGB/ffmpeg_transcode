#!/bin/bash

if [ ! -d "vcpkg" ]; then
    git clone https://github.com/microsoft/vcpkg.git
fi

cd vcpkg || exit

git reset --hard 2145fbe4f0a08de4547f391de47d88c18a01f1ba

if [ ! -f "vcpkg" ]; then
    ./bootstrap-vcpkg.sh
fi


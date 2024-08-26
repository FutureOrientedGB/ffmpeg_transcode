#!/bin/bash

cd ./project/build

make -j$(nproc)

cd -


#!/bin/bash

set -eux

mkdir -p build
cd build
cmake ..
make -j8

#!/usr/bin/env bash
mkdir Release
cd Release
cmake -DEXPERIMENTAL=1 -DCMAKE_BUILD_TYPE=Release ..
make
make package
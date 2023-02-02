#!/bin/sh

cmake --build out/build/dev-arm64

llvm-strip -x \
    -o out/build/dev-arm64/src/mypower.striped \
    out/build/dev-arm64/src/mypower

adb push out/build/dev-arm64/src/mypower.striped \
    /data/local/tmp/mypower

adb shell chmod +x /data/local/tmp/mypower

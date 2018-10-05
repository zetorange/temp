#!/bin/sh

arch=x86_64

cateyes_tests=$(dirname "$0")
cd "$cateyes_tests/../../build/tmp-macos-$arch/cateyes-core" || exit 1
. ../../cateyes-meson-env-macos-x86_64.rc
ninja || exit 1
tests/cateyes-tests "$@"

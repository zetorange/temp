#!/bin/sh

arch=x86_64

cateyes_tests=$(dirname "$0")
cd "$cateyes_tests/../../build/tmp_thin-linux-$arch/cateyes-core" || exit 1
. ../../cateyes_thin-meson-env-linux-x86_64.rc
ninja || exit 1
tests/cateyes-tests "$@"

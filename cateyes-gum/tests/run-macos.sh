#!/bin/sh

arch=x86_64

gum_tests=$(dirname "$0")
cd "$gum_tests/../../build/tmp-macos-$arch/cateyes-gum" || exit 1
. ../../cateyes-meson-env-macos-x86_64.rc
ninja || exit 1
tests/gum-tests "$@"

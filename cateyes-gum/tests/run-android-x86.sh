#!/bin/sh

arch=x86

remote_prefix=/data/local/tmp/cateyes-tests-$arch

gum_tests=$(dirname "$0")
cd "$gum_tests/../../build/tmp-android-$arch/cateyes-gum" || exit 1
. ../../cateyes-meson-env-macos-x86_64.rc
ninja || exit 1
cd tests
adb shell "mkdir $remote_prefix"
adb push gum-tests data $remote_prefix || exit 1
adb shell "$remote_prefix/gum-tests $@"

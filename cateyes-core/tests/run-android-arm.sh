#!/bin/sh

arch=arm

remote_prefix=/data/local/tmp/cateyes-tests-$arch

core_tests=$(dirname "$0")
cd "$core_tests/../../build/tmp-android-$arch/cateyes-core" || exit 1
. ../../cateyes-meson-env-macos-x86_64.rc
ninja || exit 1
cd tests
adb shell "mkdir $remote_prefix"
adb push cateyes-tests labrats ../lib/agent/cateyes-agent.so $remote_prefix || exit 1
adb shell "su -c '$remote_prefix/cateyes-tests $@'"

#!/bin/sh

arch=arm64

remote_host=iphone
remote_prefix=/var/root/cateyes-tests-$arch

core_tests=$(dirname "$0")
cd "$core_tests/../../build/tmp-ios-$arch/cateyes-core" || exit 1
. ../../cateyes-meson-env-macos-x86_64.rc
ninja || exit 1
cd tests
rsync -rLz cateyes-tests labrats ../lib/agent/cateyes-agent.dylib "$remote_host:$remote_prefix/" || exit 1
ssh "$remote_host" "$remote_prefix/cateyes-tests" "$@"

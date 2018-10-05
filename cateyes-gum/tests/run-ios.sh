#!/bin/sh

arch=arm64

remote_host=iphone
remote_prefix=/var/root/cateyes-tests-$arch

gum_tests=$(dirname "$0")
cd "$gum_tests/../../build/tmp-ios-$arch/cateyes-gum" || exit 1
. ../../cateyes-meson-env-macos-x86_64.rc
ninja || exit 1
cd tests
rsync -rLz gum-tests data "$remote_host:$remote_prefix/" || exit 1
ssh "$remote_host" "$remote_prefix/gum-tests" "$@"

#!/usr/bin/env zsh

set -e
export WORKFLOW_ROOT=/Users/runner/work/libpatchfinder/libpatchfinder/.github/workflows
export DEP_ROOT=/Users/runner/work/libpatchfinder/libpatchfinder/dep_root
export BASE=/Users/runner/work/libpatchfinder/libpatchfinder/

cd ${BASE}
export libpatchfinder_VERSION=$(git rev-list --count HEAD | tr -d '\n')
cp -RpP pkgconfig cmake-build-release-x86_64
tar cpPJf "libpatchfinder-macOS-x86_64-Build_${libpatchfinder_VERSION}-RELEASE.tar.xz" -C cmake-build-release-x86_64 libpatchfinder.a pkgconfig
cp -RpP pkgconfig cmake-build-debug-x86_64
tar cpPJf "libpatchfinder-macOS-x86_64-Build_${libpatchfinder_VERSION}-DEBUG.tar.xz" -C cmake-build-debug-x86_64 libpatchfinder.a pkgconfig
cp -RpP pkgconfig cmake-build-release-arm64
tar cpPJf "libpatchfinder-macOS-arm64-Build_${libpatchfinder_VERSION}-RELEASE.tar.xz" -C cmake-build-release-arm64 libpatchfinder.a pkgconfig
cp -RpP pkgconfig cmake-build-debug-arm64
tar cpPJf "libpatchfinder-macOS-arm64-Build_${libpatchfinder_VERSION}-DEBUG.tar.xz" -C cmake-build-debug-arm64 libpatchfinder.a pkgconfig

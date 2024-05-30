#!/usr/bin/env bash

set -e
export TMPDIR=/tmp
export WORKFLOW_ROOT=${TMPDIR}/Builder/repos/libpatchfinder/.github/workflows
export DEP_ROOT=${TMPDIR}/Builder/repos/libpatchfinder/dep_root
export BASE=${TMPDIR}/Builder/repos/libpatchfinder/

cd ${BASE}
export libpatchfinder_VERSION=$(git rev-list --count HEAD | tr -d '\n')
cd ${WORKFLOW_ROOT}
echo "libpatchfinder-Linux-x86_64-Build_${libpatchfinder_VERSION}-RELEASE.tar.xz" > name1.txt
echo "libpatchfinder-Linux-x86_64-Build_${libpatchfinder_VERSION}-DEBUG.tar.xz" > name2.txt
cp -RpP "${BASE}/cmake-build-release-x86_64/libpatchfinder.a" libpatchfinder.a
cp -RpP "${BASE}/pkgconfig" .
rm pkgconfig/*.in
tar cpPJvf "libpatchfinder1.tar.xz" libpatchfinder.a pkgconfig
cp -RpP "${BASE}/cmake-build-debug-x86_64/libpatchfinder.a" libpatchfinder.a
tar cpPJvf "libpatchfinder2.tar.xz" libpatchfinder.a pkgconfig

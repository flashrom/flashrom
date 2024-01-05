#!/usr/bin/env bash

set -o xtrace
set -o errexit

RPMBUILD=${HOME}/rpmbuild

microdnf update -y
microdnf install -y dnf epel-release almalinux-release-devel
dnf install -y rpm-build rpmdevtools
dnf config-manager --set-enabled crb

mkdir -p ${RPMBUILD}/SPECS
mv .github/flashrom.spec ${RPMBUILD}/SPECS/flashrom.spec
dnf builddep -y ${RPMBUILD}/SPECS/flashrom.spec
spectool -g -R ${RPMBUILD}/SPECS/flashrom.spec
rpmbuild -bb ${RPMBUILD}/SPECS/flashrom.spec

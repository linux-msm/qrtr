#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
#
# Copyright (c) 2021 Canonical Ltd.
# Copyright (c) 2023 Linaro Ltd
# Author: Krzysztof Kozlowski <krzysztof.kozlowski@linaro.org>
#                             <krzk@kernel.org>
#

set -ex

PKGS_CC="gcc"
case $CC in
	clang*)
		PKGS_CC="clang"
	;;
esac

dnf -y install \
	meson \
	pkg-config \
	libudev-devel \
	systemd-devel \
	systemd-libs \
	$PKGS_CC

echo "Install finished: $0"

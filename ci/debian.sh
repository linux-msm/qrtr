#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
#
# Copyright (c) 2021 Canonical Ltd.
# Copyright (c) 2023 Linaro Ltd
# Author: Krzysztof Kozlowski <krzysztof.kozlowski@linaro.org>
#                             <krzk@kernel.org>
#

set -ex

apt update

# Some distros (e.g. Ubuntu Hirsute) might pull tzdata which asks questions
export DEBIAN_FRONTEND=noninteractive DEBCONF_NONINTERACTIVE_SEEN=true

# Choose some random place in Europe
echo "tzdata tzdata/Areas select Europe
tzdata tzdata/Zones/Europe select Berlin
" > /tmp/tzdata-preseed.txt
debconf-set-selections /tmp/tzdata-preseed.txt

PKGS_CC="build-essential"
case $CC in
	clang*)
		PKGS_CC="clang"
	;;
esac

apt install -y --no-install-recommends \
	pkg-config \
	meson \
	systemd \
	libc6-dev \
	$PKGS_CC

# Thanks debian
apt install -y --no-install-recommends systemd-dev || true

echo "Install finished: $0"

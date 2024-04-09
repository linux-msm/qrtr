#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
#
# Copyright (c) 2018-2020 Petr Vorel <pvorel@suse.cz>
# Copyright (c) 2021 Canonical Ltd.
# Copyright (c) 2023 Linaro Ltd
# Author: Krzysztof Kozlowski <krzysztof.kozlowski@linaro.org>
#                             <krzk@kernel.org>
#

set -ex

dpkg --add-architecture i386
apt update

# gcc-multilib are also needed for clang 32-bit builds
PKGS_CC="gcc-multilib"

apt install -y --no-install-recommends \
	linux-libc-dev:i386

apt install -y --no-install-recommends \
	$PKGS_CC

echo "Install finished: $0"

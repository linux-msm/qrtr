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

if [ -z "$ARCH" ]; then
	echo "missing \$ARCH!" >&2
	exit 1
fi

dpkg --add-architecture $ARCH
apt update

apt install -y --no-install-recommends \
	libc6-dev:${ARCH} \
	gcc-`dpkg-architecture -a ${ARCH} -q DEB_TARGET_GNU_TYPE`

# Thanks debian
apt install -y --no-install-recommends systemd-dev:${ARCH} -a ${ARCH} || true

echo "Install finished: $0"

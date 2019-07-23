#!/bin/bash
export CROSS_COMPILE=/opt/aarch64_cortex-a53_marvell/bin/aarch64-openwrt-linux-
export STAGING_DIR=.
make mvebu_espressobin-88f3720_defconfig
make DEVICE_TREE=armada-3720-espressobin  V=1

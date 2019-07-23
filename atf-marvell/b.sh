#!/bin/bash
export BL33=../u-boot-marvell/u-boot.bin
export STAGING_DIR=/
export CROSS_COMPILE=/opt/aarch64_cortex-a53_marvell/bin/aarch64-openwrt-linux-
make -j1 DEBUG=1 SECONDARY_BOOTIMG=0 USE_COHERENT_MEM=0 LOG_LEVEL=20 SECURE=0 CLOCKSPRESET=CPU_1000_DDR_800 DDR_TOPOLOGY=1 BOOTDEV=EMMCNORM PARTNUM=1 WTP=../A3700-utils-marvell PLAT=a3700 all fip

#!/bin/bash

BASE=`pwd`

cd $BASE/u-boot-marvell
./b.sh

cd $BASE/atf-marvell
./b.sh

cp $BASE/atf-marvell/build/a3700/debug/flash-image.bin $BASE/

#!/bin/bash

export CROSS_COMPILE=/android-ndk-r7/toolchains/arm-linux-androideabi-4.4.3/prebuilt/darwin-x86/bin/arm-linux-androideabi-
export CFLAGS=
export LDFLAGS=

make ARCH=arm -j8

mkbootfs ./ramdisk | gzip > ramdisk.cpio.gz
mkbootimg --kernel arch/arm/boot/zImage --ramdisk ramdisk.cpio.gz --cmdline "console=ttyO2,115200n8 mem=463M@0x80000000 init=/init vram=5M omapfb.vram=0:5M" --pagesize 4096 --base 0x80000000 -o boot.img

ls -lh boot.img

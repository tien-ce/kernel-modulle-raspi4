#!/bin/bash

# 1. Toolchain Path
# Adds the Yocto cross-compiler binaries to the system PATH
export PATH=/data200/yocto-build-64/tmp/work/raspberrypi4_lsmy-poky-linux/linux-raspberrypi/6.6.63+git/recipe-sysroot-native/usr/bin/aarch64-poky-linux:$PATH

# 2. Architecture and Cross-Compiler Variables
# Defines the target architecture and the toolchain prefix
export ARCH=arm64
export CROSS_COMPILE=aarch64-poky-linux-

# 3. Kernel Directory Definitions
# KERNEL_SRC: Path to the shared kernel source code
# KERNEL_OUT: Path to the kernel build artifacts (containing .config and generated headers)
export KERNEL_SRC=/data200/yocto-build-64/tmp-glibc/work-shared/raspberrypi4-lsmy/kernel-source
export KERNEL_OUT=/data200/yocto-build-64/tmp/work/raspberrypi4_lsmy-poky-linux/linux-raspberrypi/6.6.63+git/linux-raspberrypi4_lsmy-standard-build

# 4. Compiler Setup with Sysroot
# Configures the C compiler with specific CPU optimization and the Yocto target sysroot
export CC="aarch64-poky-linux-gcc -mcpu=cortex-a72+crc --sysroot=/data200/yocto-build-64/tmp/work/raspberrypi4_lsmy-poky-linux/linux-raspberrypi/6.6.63+git/recipe-sysroot"

echo "Environment for Raspberry Pi 4 Kernel Module development is successfully initialized."

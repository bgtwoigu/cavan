ROOT_PATH = $(shell pwd)
BUILD_PATH = $(ROOT_PATH)/build
PACKAGE_PATH = $(ROOT_PATH)/package
PATCH_PATH = $(ROOT_PATH)/patch
SCRIPT_PATH = $(BUILD_PATH)/script

BUILD_CORE = $(BUILD_PATH)/core
BUILD_TOOLCHIAN = $(BUILD_PATH)/toolchian
BUILD_ROOTFS = $(BUILD_PATH)/rootfs
BUILD_UTILS = $(BUILD_PATH)/utils

WORK_PATH = $(HOME)/cavan-toolchian
SRC_PATH = $(WORK_PATH)/src
UTILS_PATH = $(WORK_PATH)/utils
DECOMP_PATH = $(WORK_PATH)/decomp
MARK_PATH = $(WORK_PATH)/mark
OUT_PATH = $(WORK_PATH)/out

PYTHON_PARSER = $(SCRIPT_PATH)/parser2.py
XML_PACKAGE = $(BUILD_CORE)/package.xml
MAKEFILE_PACKAGE = $(OUT_PATH)/package.mk
MAKEFILE_DOWNLOAD = $(OUT_PATH)/download.mk

DOWNLOAD_PATH = $(word 1,$(wildcard /source /work/source $(HOME)/source $(HOME)/work/source))
ifeq ($(DOWNLOAD_PATH),)
DOWNLOAD_PATH = $(HOME)/download
endif

LOWEST_KERNEL_VERSION = 2.6.15
GCC_VERSION_LIST = $(patsubst config-%.mk,%,$(notdir $(wildcard build/toolchian/config-*.mk)))

ifeq ($(filter $(GCC_VERSION),$(GCC_VERSION_LIST)),)
$(error do not support gcc-$(GCC_VERSION))
endif

include build/toolchian/config-$(GCC_VERSION).mk

export BINUTILS_VERSION GCC_VERSION GLIBC_VERSION KERNEL_VERSION LOWEST_KERNEL_VERSION
export GMP_VERSION MPFR_VERSION MPC_VERSION

CAVAN_TARGET_PLAT = $(CAVAN_TARGET_ARCH)-cavan-linux-$(CAVAN_TARGET_EABI)

CAVAN_HOST_ARCH = $(CAVAN_BUILD_ARCH)
TOOLCHIAN_PREFIX = $(WORK_PATH)/toolchian

ifeq ($(CAVAN_HOST_ARCH),$(CAVAN_TARGET_ARCH))
TOOLCHIAN_NAME = $(CAVAN_TARGET_PLAT)-$(GCC_VERSION)
else
TOOLCHIAN_NAME = $(CAVAN_TARGET_PLAT)-$(GCC_VERSION)-$(CAVAN_HOST_ARCH)
endif

ifneq ($(CAVAN_TARGET_MX32),)
TOOLCHIAN_NAME := $(TOOLCHIAN_NAME)-$(CAVAN_TARGET_MX32)
endif

ifneq ($(filter amd64 x86_64,$(CAVAN_HOST_ARCH)),)
CAVAN_HOST_ARCH_32 = i686
endif

TOOLCHIAN_PATH = $(TOOLCHIAN_PREFIX)/$(TOOLCHIAN_NAME)
SYSROOT_PATH = $(TOOLCHIAN_PATH)/sysroot

ROOTFS_PATH = $(WORK_PATH)/rootfs/$(CAVAN_TARGET_ARCH)
OUT_ROOTFS = $(OUT_PATH)/rootfs/$(CAVAN_TARGET_ARCH)
OUT_UTILS = $(OUT_PATH)/utils
MARK_ROOTFS = $(MARK_PATH)/rootfs/$(CAVAN_TARGET_ARCH)
MARK_UTILS = $(MARK_PATH)/utils
MARK_DOWNLOAD = $(MARK_PATH)/download

MARK_ROOTFS_READY = $(MARK_ROOTFS)/ready
MARK_DOWNLOAD_READY = $(MARK_DOWNLOAD)/ready
MARK_UTILS_READY1 = $(MARK_UTILS)/ready1
MARK_UTILS_READY2 = $(MARK_UTILS)/ready2

MAKEFILE_TOOLCHIAN = $(BUILD_TOOLCHIAN)/main.mk
MAKEFILE_ROOTFS = $(BUILD_ROOTFS)/main.mk
MAKEFILE_UTILS = $(BUILD_UTILS)/main.mk
MAKEFILE_DEFINES = $(BUILD_CORE)/defines.mk
BASH_DOWNLOAD = $(SCRIPT_PATH)/download.sh

MARK_HOST_APPS = $(MARK_UTILS)/environment

ifeq ($(CAVAN_HOST_ARCH),$(CAVAN_BUILD_ARCH))
define export_path
$(foreach path,/ /usr/ /usr/local/,$(eval export PATH := $1$(path)sbin:$1$(path)bin:$(PATH)))
endef
else
define export_path
$(warning export path $1, nothing to be done)
endef
endif

$(call export_path,$(UTILS_PATH))
$(call export_path,$(TOOLCHIAN_PATH))

DOWNLOAD_COMMAND = bash $(BASH_DOWNLOAD)

export CAVAN_TARGET_PLAT ROOT_PATH PACKAGE_PATH BUILD_PATH PATCH_PATH SCRIPT_PATH
export SRC_PATH ROOTFS_PATH UTILS_PATH DECOMP_PATH PATH DOWNLOAD_PATH
export OUT_PATH OUT_UTILS OUT_ROOTFS
export BUILD_CORE BUILD_TOOLCHIAN BUILD_ROOTFS BUILD_UTILS
export MARK_PATH MARK_ROOTFS MARK_UTILS MARK_DOWNLOAD
export MARK_ROOTFS_READY MARK_UTILS_READY1 MARK_UTILS_READY2
export MAKEFILE_DEFINES MAKEFILE_TOOLCHIAN MAKEFILE_PACKAGE
export PYTHON_PARSER XML_APPLICATION BASH_DOWNLOAD DOWNLOAD_COMMAND
export TOOLCHIAN_PREFIX TOOLCHIAN_NAME TOOLCHIAN_PATH SYSROOT_PATH
export CAVAN_HOST_ARCH CAVAN_HOST_ARCH_32

$(info ============================================================)
$(info PACKAGE_PATH = $(PACKAGE_PATH))
$(info PATCH_PATH = $(PATCH_PATH))
$(info DOWNLOAD_PATH = $(DOWNLOAD_PATH))
$(info ============================================================)

include $(MAKEFILE_DEFINES)

all:
	echo 1111111111111111111111111111

download: $(MARK_DOWNLOAD_READY)
	@echo make $@ successfully

$(MARK_DOWNLOAD_READY):
	$(Q)$(call generate_makefile,download,$(XML_PACKAGE),$(MAKEFILE_DOWNLOAD))
	$(Q)+make -f $(MAKEFILE_DOWNLOAD)
	$(Q)$(call generate_mark)
	@echo make $@ successfully

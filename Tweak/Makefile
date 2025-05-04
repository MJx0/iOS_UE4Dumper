TARGET := iphone:clang:latest:14.0

THEOS_PACKAGE_SCHEME = rootless

ifeq ($(THEOS_PACKAGE_SCHEME),rootless)
THEOS_PACKAGE_DIR = packages/rootless
else ifeq ($(THEOS_PACKAGE_SCHEME),roothide)
THEOS_PACKAGE_DIR = packages/roothide
else
THEOS_PACKAGE_DIR = packages/rootful
endif

#THEOS_DEVICE_IP =
ARCHS = arm64 arm64e
DEBUG = 0
FINALPACKAGE = 1

# set to 1 if you want to use memcpy instead of vm_read_overwrite
# memcpy will speed up the dumping process by a lot but may crash on any invalid address
# use it if you trust your addresses
RPM_USE_MEMCPY = 0

SKIPPED_WARNINGS = -Wno-unused-parameter -Wno-unused-variable -Wno-deprecated-declarations

ZIP_ARCHIVE_DEFINES = -DHAVE_INTTYPES_H -DHAVE_PKCRYPT -DHAVE_STDINT_H -DHAVE_WZAES -DHAVE_ZLIB

KITTYMEMORY_PATH = ../deps/KittyMemory/KittyMemory
KITTYMEMORY_SRC = $(wildcard $(KITTYMEMORY_PATH)/*.cpp)

DEPS_PATH = ../deps
DEPS_SRC = $(KITTYMEMORY_SRC) $(DEPS_PATH)/fmt/format.cc \
$(DEPS_PATH)/SSZipArchive/SSZipArchive.m $(wildcard $(DEPS_PATH)/SSZipArchive/minizip/*.c)

include $(THEOS)/makefiles/common.mk

TWEAK_NAME = UEDumper

$(TWEAK_NAME)_FILES = src/Tweak.mm \
src/Utils/AlertUtils.mm src/Utils/KittyAlertView.mm src/Utils/memory.cpp src/Utils/BufferFmt.cpp \
$(wildcard src/Core/*.cpp) $(DEPS_SRC)

$(TWEAK_NAME)_CFLAGS = -fobjc-arc $(ZIP_ARCHIVE_DEFINES) $(SKIPPED_WARNINGS)

$(TWEAK_NAME)_CCFLAGS = -std=c++17 -O2 $(SKIPPED_WARNINGS) -I$(DEPS_PATH) -I$(KITTYMEMORY_PATH) -DkNO_KEYSTONE -DkNO_SUBSTRATE
ifeq ($(RPM_USE_MEMCPY), 1)
$(TWEAK_NAME)_CCFLAGS += -DRPM_USE_MEMCPY
endif

$(TWEAK_NAME)_LDFLAGS = -lz -liconv -ldl

$(TWEAK_NAME)_FRAMEWORKS = UIKit Foundation Security Network QuartzCore

include $(THEOS_MAKE_PATH)/tweak.mk

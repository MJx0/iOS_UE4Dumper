TARGET := iphone:clang:latest:10.0

#THEOS_PACKAGE_SCHEME = rootless

ifeq ($(THEOS_PACKAGE_SCHEME),rootless)
THEOS_PACKAGE_DIR = packages/rootless
else
THEOS_PACKAGE_DIR = packages/rootful
endif

INSTALL_TARGET_PROCESSES = \
SolarlandClient \
 DeadByDaylight \
  ShooterGame \
   ShadowTrackerExtra \
    PESmobile \
     XGame

#THEOS_DEVICE_IP =
ARCHS = arm64 arm64e
DEBUG = 0
FINALPACKAGE = 1

# set to 1 if you want to use memcpy instead of vm_read_overwrite
# memcpy will speed up the dumping process by a lot but may crash on any invalid address
# use it if you trust your addresses
RPM_USE_MEMCPY = 0

ZIP_ARCHIVE_DEFINES = -DHAVE_INTTYPES_H -DHAVE_PKCRYPT -DHAVE_STDINT_H -DHAVE_WZAES -DHAVE_ZLIB

KITTYMEMORY_PATH = ../deps/KittyMemory/KittyMemory
KITTYMEMORY_SRC = $(wildcard $(KITTYMEMORY_PATH)/*.cpp)

include $(THEOS)/makefiles/common.mk

TWEAK_NAME = UE4Dumper

$(TWEAK_NAME)_FILES = src/Tweak.mm src/AlertUtils.mm src/DumpTransfer.mm src/DumpTransferUI.mm \
$(wildcard src/Core/*.cpp) \
$(KITTYMEMORY_SRC) \
$(wildcard includes/fmt/format.cc) \
$(wildcard includes/SCLAlertView/*.m) \
$(wildcard includes/GCDAsyncSocket/*.m) \
$(wildcard includes/SSZipArchive/SSZipArchive.m) $(wildcard includes/SSZipArchive/minizip/*.c)

$(TWEAK_NAME)_CFLAGS = -fobjc-arc $(ZIP_ARCHIVE_DEFINES)

$(TWEAK_NAME)_CCFLAGS = -std=c++17 -Iincludes -I$(KITTYMEMORY_PATH) -O2 -DkNO_KEYSTONE -DkNO_SUBSTRATE
ifeq ($(RPM_USE_MEMCPY), 1)
$(TWEAK_NAME)_CCFLAGS += -DRPM_USE_MEMCPY
endif

$(TWEAK_NAME)_LDFLAGS = -lz -liconv -ldl

$(TWEAK_NAME)_FRAMEWORKS = UIKit Foundation Security Network

include $(THEOS_MAKE_PATH)/tweak.mk
TARGET := iphone:clang:latest:10.0
INSTALL_TARGET_PROCESSES = \
AClient \
 DeadByDaylight \
  ShooterGame \
   ShadowTrackerExtra \
    PESmobile \
     XGame

#THEOS_DEVICE_IP =
ARCHS = arm64 arm64e
DEBUG = 0
FINALPACKAGE = 1

IGNORE_WARNINGS = 0

# set to 1 if you want to use memcpy instead of vm_read_overwrite
# memcpy will speed up the dumping process by a lot but may crash on any invalid address
# use it if you trust your addresses
RPM_USE_MEMCPY = 0

ZIP_ARCHIVE_DEFINES = -DHAVE_INTTYPES_H -DHAVE_PKCRYPT -DHAVE_STDINT_H -DHAVE_WZAES -DHAVE_ZLIB

include $(THEOS)/makefiles/common.mk

TWEAK_NAME = UE4Dumper

$(TWEAK_NAME)_FILES = Tweak/src/Tweak.mm Tweak/src/AlertUtils.mm Tweak/src/DumpTransfer.mm Tweak/src/DumpTransferUI.mm \
$(wildcard Tweak/src/Core/*.cpp) \
$(wildcard Tweak/includes/KittyMemory/*.cpp) \
$(wildcard Tweak/includes/fmt/format.cc) \
$(wildcard Tweak/includes/SCLAlertView/*.m) \
$(wildcard Tweak/includes/GCDAsyncSocket/*.m) \
$(wildcard Tweak/includes/SSZipArchive/SSZipArchive.m) $(wildcard Tweak/includes/SSZipArchive/minizip/*.c)

$(TWEAK_NAME)_CFLAGS = -fobjc-arc $(ZIP_ARCHIVE_DEFINES)
ifeq ($(IGNORE_WARNINGS), 1)
$(TWEAK_NAME)_CFLAGS += -w
endif

$(TWEAK_NAME)_CCFLAGS = -std=c++17 -ITweak/includes -O3 -D_NO_SUBSTRATE_LINK
ifeq ($(IGNORE_WARNINGS), 1)
$(TWEAK_NAME)_CCFLAGS += -w
endif
ifeq ($(RPM_USE_MEMCPY), 1)
$(TWEAK_NAME)_CCFLAGS += -DRPM_USE_MEMCPY
endif

$(TWEAK_NAME)_LDFLAGS = -lz -liconv -ldl

$(TWEAK_NAME)_FRAMEWORKS = UIKit Foundation Security Network

include $(THEOS_MAKE_PATH)/tweak.mk
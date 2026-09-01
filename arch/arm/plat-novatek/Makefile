#
# Makefile for the linux kernel.
#

ccflags-$(CONFIG_ARCH_NVT_IVOT_V7) := -I$(srctree)/arch/arm/plat-novatek/include

obj-$(CONFIG_NVT_IVOT_PLAT_NA51089)         += plat-na51089/reset.o plat-na51089/pll_preset.o plat-na51089/fastboot_preload.o plat-na51089/outstanding_control.o nvt-sramctl.o
obj-$(CONFIG_NVT_IVOT_PLAT_NA51068)         += plat-na51068/reset.o plat-na51068/nvt_jiffies.o plat-na51068/pll_preset.o nvt-sramctl.o
obj-$(CONFIG_NVT_IVOT_PLAT_NA51055)         += plat-na51055/reset.o plat-na51055/pll_preset.o plat-na51055/fastboot_preload.o plat-na51055/outstanding_control.o nvt-sramctl.o

nvt-platform-$(CONFIG_NVT_IVOT_PLAT_NA51089)		:= plat-na51089
nvt-platform-$(CONFIG_NVT_IVOT_PLAT_NA51068)		:= plat-na51068
nvt-platform-$(CONFIG_NVT_IVOT_PLAT_NA51055)		:= plat-na51055

ifeq ($(CONFIG_NVT_IVOT_PLAT_NA51068), y)
	nvt_otp_protect := na51068
else ifeq ($(CONFIG_NVT_IVOT_PLAT_NA51089), y)
	nvt_otp_protect := na51089
else
	nvt_otp_protect := na51055
endif

PLATFORM_DIR   := $(nvt-platform-y)
MYKERNEL_PATH = $(realpath $(srctree))
PLATFORM_INC_SYMBOLIC =  $(MYKERNEL_PATH)/arch/arm/plat-novatek/include/plat
PLATFORM_INC_TARGET = $(PLATFORM_DIR)
PLATFORM_SRC_SYMBOLIC = $(MYKERNEL_PATH)/arch/arm/plat-novatek/plat
PLATFORM_SRC_TARGET = $(PLATFORM_DIR)
VOS_SYMBOLIC = $(MYKERNEL_PATH)/drivers/soc/nvt/nvt_hdal/vos
VOS_TARGET = $(realpath $(NVT_VOS_DIR))
HDAL_SYMBOLIC = $(MYKERNEL_PATH)/drivers/soc/nvt/nvt_hdal/hdal
HDAL_TARGET = $(realpath $(NVT_HDAL_DIR))
OTP_DEFAULT_DIR = $(MYKERNEL_PATH)/drivers/soc/nvt/otp
OTP_O_DIR = $(MYKERNEL_PATH)/$(O)/drivers/soc/nvt/otp
OTP_KBUILD_OUTPUT_DIR = $(MYKERNEL_PATH)/$(KBUILD_OUTPUT)/drivers/soc/nvt/otp

$(PLATFORM_INC_SYMBOLIC): FORCE
ifneq ($(notdir $(realpath $(PLATFORM_INC_SYMBOLIC))), $(PLATFORM_INC_TARGET))
	@echo Creating $(PLATFORM_INC_SYMBOLIC)
	@yes | rm -f $(PLATFORM_INC_SYMBOLIC) && ln -fsn $(PLATFORM_INC_TARGET) $(PLATFORM_INC_SYMBOLIC)
endif
ifneq ($(notdir $(realpath $(PLATFORM_SRC_SYMBOLIC))), $(PLATFORM_SRC_TARGET))
	@echo Creating $(PLATFORM_SRC_SYMBOLIC)
	@yes | rm -f $(PLATFORM_SRC_SYMBOLIC) && ln -fsn $(PLATFORM_SRC_TARGET) $(PLATFORM_SRC_SYMBOLIC)
endif
ifneq ($(realpath $(VOS_SYMBOLIC)), $(VOS_TARGET))
	@echo Creating $(VOS_SYMBOLIC)
	@yes | rm -f $(VOS_SYMBOLIC) &&	ln -fsn $(VOS_TARGET) $(VOS_SYMBOLIC)
endif
ifneq ($(realpath $(HDAL_SYMBOLIC)), $(HDAL_TARGET))
	@echo Creating $(HDAL_SYMBOLIC)
	@yes | rm -f $(HDAL_SYMBOLIC) && ln -fsn $(HDAL_TARGET) $(HDAL_SYMBOLIC)
endif
ifneq ($(O),)
	@echo Copying $(OTP_O_DIR)/$(nvt_otp_protect)
	@mkdir -p $(OTP_O_DIR)/$(nvt_otp_protect) && cp -arfv $(OTP_DEFAULT_DIR)/$(nvt_otp_protect) $(OTP_O_DIR)/
endif
ifneq ($(KBUILD_OUTPUT),)
	@echo Copying $(OTP_KBUILD_OUTPUT_DIR)/$(nvt_otp_protect)
	@mkdir -p $(OTP_KBUILD_OUTPUT_DIR)/$(nvt_otp_protect) && cp -arfv $(OTP_DEFAULT_DIR)/$(nvt_otp_protect) $(OTP_KBUILD_OUTPUT_DIR)/
endif

prepare: $(PLATFORM_INC_SYMBOLIC)
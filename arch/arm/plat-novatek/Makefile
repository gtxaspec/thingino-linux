#
# Makefile for the linux kernel.
#

ccflags-$(CONFIG_ARCH_NVT_IVOT_V7) := -I$(srctree)/arch/arm/plat-novatek/include

obj-$(CONFIG_NVT_IVOT_PLAT_NA51068)			+= plat-na51068/reset.o plat-na51068/nvt_jiffies.o plat-na51068/pll_preset.o nvt-sramctl.o
obj-$(CONFIG_NVT_IVOT_PLAT_NA51055)			+= plat-na51055/reset.o plat-na51055/pll_preset.o nvt-sramctl.o

nvt-platform-$(CONFIG_NVT_IVOT_PLAT_NA51068)		:= plat-na51068
nvt-platform-$(CONFIG_NVT_IVOT_PLAT_NA51055)		:= plat-na51055

PLATFORM_DIR   := $(nvt-platform-y)

define create-platform-symlink
  MYKERNEL_PATH=$(shell cd $(srctree); pwd);											\
  yes | rm -f $$MYKERNEL_PATH/arch/arm/plat-novatek/include/plat;								\
  ln -fsn $$MYKERNEL_PATH/arch/arm/plat-novatek/include/$(PLATFORM_DIR) $$MYKERNEL_PATH/arch/arm/plat-novatek/include/plat;	\
  yes | rm -f $$MYKERNEL_PATH/arch/arm/plat-novatek/plat;									\
  ln -fsn $$MYKERNEL_PATH/arch/arm/plat-novatek/$(PLATFORM_DIR) $$MYKERNEL_PATH/arch/arm/plat-novatek/plat;			\
  if [ -d $(NVT_VOS_DIR) ]; then 												\
	yes | rm -f $$MYKERNEL_PATH/drivers/soc/nvt/nvt_hdal/vos;								\
	ln -fsn $(NVT_VOS_DIR) $$MYKERNEL_PATH/drivers/soc/nvt/nvt_hdal/vos;							\
  fi;																\
  if [ -d $(NVT_HDAL_DIR) ]; then 												\
	yes | rm -f $$MYKERNEL_PATH/drivers/soc/nvt/nvt_hdal/hdal;								\
	ln -fsn $(NVT_HDAL_DIR) $$MYKERNEL_PATH/drivers/soc/nvt/nvt_hdal/hdal;							\
  fi;
endef

arch/arm/plat-novatek/include/plat: FORCE
	$(Q) $(create-platform-symlink)

prepare: arch/arm/plat-novatek/include/plat

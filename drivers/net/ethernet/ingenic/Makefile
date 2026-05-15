
####################################################
# Explanation for EXTRA_FLAGS
# -DDEBUG		     => Enable the debug trace for driver
# -DIPC_OFFLOAD		     => Enables the IP and TCP checksum offloading feature in HW (IPV4 only)
# -DENH_DESC		     => Enables Enhanced Descriptors
# -DENH_DESC_8W              => Enable Enhanced Descriptors of 8words or else Descriptor will be of 4words.
####################################################


#with the below flags enables Debug messages

#EXTRA_CFLAGS += -DDEBUG
#EXTRA_CFLAGS +=-DDEBUG -DIPC_OFFLOAD
#EXTRA_CFLAGS += -DDEBUG -DIPC_OFFLOAD
#EXTRA_CFLAGS += -DIPC_OFFLOAD


#Enhanced Descriptor
#EXTRA_CFLAGS += -DENH_DESC
#EXTRA_CFLAGS += -DENH_DESC -DIPC_OFFLOAD
EXTRA_CFLAGS += -DENH_DESC -DENH_DESC_8W
#EXTRA_CFLAGS += -DDEBUG -DENH_DESC -DENH_DESC_8W
#EXTRA_CFLAGS += -DDEBUG -DENH_DESC -DENH_DESC_8W -DIPC_OFFLOAD
#EXTRA_CFLAGS += -DENH_DESC -DENH_DESC_8W -DIPC_OFFLOAD
#EXTRA_CFLAGS += -DENH_DESC -DENH_DESC_8W -DAVB_SUPPORT  -DDEBUG
#EXTRA_CFLAGS += -DENH_DESC -DENH_DESC_8W -DAVB_SUPPORT
#EXTRA_CFLAGS += -DENH_DESC -DENH_DESC_8W

ifeq ($(CONFIG_SOC_T30), y)
jzmac-objs += jz_mac_t30_v13.o synopGMAC_plat.o synopGMAC_Dev.o
obj-$(CONFIG_JZ_MAC) += jzmac.o
endif

ifeq ($(CONFIG_SOC_T31), y)
jzmac-objs += jz_mac_t30_v13.o synopGMAC_plat.o synopGMAC_Dev.o
obj-$(CONFIG_JZ_MAC) += jzmac.o
endif

#obj-$(CONFIG_JZ_MAC) += synopGMAC_plat.o synopGMAC_Dev.o
#obj-$(CONFIG_JZ_MAC_V12) += jz_mac.o

ifeq ($(CONFIG_SOC_T20), y)
jzmac-objs += jz_mac_v13.o synopGMAC_plat.o synopGMAC_Dev.o
obj-$(CONFIG_JZ_MAC) += jzmac.o
endif

ifeq ($(CONFIG_SOC_T21), y)
jzmac-objs += jz_mac_t21_v13.o synopGMAC_plat.o synopGMAC_Dev.o
obj-$(CONFIG_JZ_MAC) += jzmac.o
endif

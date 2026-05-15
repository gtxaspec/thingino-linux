obj-y += ingenic_vpu.o
obj-$(CONFIG_JZ_VPU_KERN_ENCODE) += vpu_common.o
obj-$(CONFIG_JZ_VPU_KERN_ENCODE) += src/
subdir-ccflags-y += -Idrivers/video/soc_vpu/kernel_encode/include
subdir-ccflags-$(CONFIG_JZ_VPU_KERN_ENCODE_H265) += -Idrivers/video/soc_vpu/kernel_encode/include/h265
subdir-ccflags-$(CONFIG_JZ_VPU_KERN_ENCODE_H264) += -Idrivers/video/soc_vpu/kernel_encode/include/h264
subdir-ccflags-$(CONFIG_JZ_VPU_KERN_ENCODE_JPEG) += -Idrivers/video/soc_vpu/kernel_encode/include/jpeg

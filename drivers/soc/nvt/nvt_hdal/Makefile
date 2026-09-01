#
# Makefile for the hdal built-in
#

VOS_DRIVER_SRC_DIR = vos/drivers/source
KDRV_SRC_DIR = hdal/drivers/k_driver/source
KFLOW_SRC_DIR = hdal/drivers/k_flow/source
VENDOR_ISP_DIR = hdal/vendor/isp/drivers/source
VENDOR_MEDIA_DIR = hdal/vendor/media/drivers/source

# to avoid make clean error (when make cleaning, CONFIG_NVT_VOS_KWRAP != y)
ifeq ($(CONFIG_NVT_VOS_KWRAP),y)
obj-$(CONFIG_NVT_VOS_KWRAP) += $(VOS_DRIVER_SRC_DIR)/kwrap/
endif

# to avoid make clean error (when make cleaning, CONFIG_NVT_HDAL_KDRV_BUILTIN != y)
ifeq ($(CONFIG_NVT_HDAL_KDRV_BUILTIN),y)
obj-$(CONFIG_NVT_HDAL_KDRV_BUILTIN) += $(KDRV_SRC_DIR)/kdrv_builtin/
obj-$(CONFIG_NVT_HDAL_KDRV_COMM) += $(KDRV_SRC_DIR)/comm/
obj-$(CONFIG_NVT_HDAL_KDRV_GFX2D) += $(KDRV_SRC_DIR)/kdrv_gfx2d/
obj-$(CONFIG_NVT_HDAL_KDRV_VIDEOCAPTURE) += $(KDRV_SRC_DIR)/kdrv_videocapture/
obj-$(CONFIG_NVT_HDAL_KDRV_VIDEOPROCESS) += $(KDRV_SRC_DIR)/kdrv_videoprocess/
obj-$(CONFIG_NVT_HDAL_KDRV_VIDEOCODEC) += $(KDRV_SRC_DIR)/kdrv_videocodec/
obj-$(CONFIG_NVT_HDAL_KDRV_AUDIO_COMMON) += $(KDRV_SRC_DIR)/kdrv_audioio/audio_common/
obj-$(CONFIG_NVT_HDAL_KDRV_AUDIO) += $(KDRV_SRC_DIR)/kdrv_audioio/kdrv_audio/
obj-$(CONFIG_NVT_HDAL_KFLOW_COMMON) += $(KFLOW_SRC_DIR)/kflow_common/
obj-$(CONFIG_NVT_HDAL_KFLOW_VIDEOCAPTURE) += $(KFLOW_SRC_DIR)/kflow_videocapture/
obj-$(CONFIG_NVT_HDAL_KFLOW_VIDEOSPRITE) += $(KFLOW_SRC_DIR)/kflow_gfx/videosprite/
obj-$(CONFIG_NVT_HDAL_KFLOW_VIDEOPROCESS) += $(KFLOW_SRC_DIR)/kflow_videoprocess/
obj-$(CONFIG_NVT_HDAL_KFLOW_VIDEOENC) += $(KFLOW_SRC_DIR)/kflow_videoenc/unit/
obj-$(CONFIG_NVT_HDAL_KFLOW_GFX2D) += $(KFLOW_SRC_DIR)/kflow_gfx/
obj-$(CONFIG_NVT_HDAL_KFLOW_AUDIOCAPTURE) += $(KFLOW_SRC_DIR)/kflow_audiocapture/unit/
obj-$(CONFIG_NVT_HDAL_KDRV_VIDEOCODEC) += $(VENDOR_MEDIA_DIR)/nvt_vencrc/
obj-$(CONFIG_NVT_HDAL_ISP_ISP) += $(VENDOR_ISP_DIR)/isp/
obj-$(CONFIG_NVT_HDAL_ISP_AE) += $(VENDOR_ISP_DIR)/ae/
obj-$(CONFIG_NVT_HDAL_ISP_AWB) += $(VENDOR_ISP_DIR)/awb/
obj-$(CONFIG_NVT_HDAL_ISP_IQ) += $(VENDOR_ISP_DIR)/iq/
endif

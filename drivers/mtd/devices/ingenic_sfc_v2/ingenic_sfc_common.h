#ifndef ingenic_SFC_COMMON_H
#define ingenic_SFC_COMMON_H
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include "sfc.h"
#include "sfc_flash.h"

#define SFC_IOTC_CHECK 'S'
#define SFC_READ_SR   _IO(SFC_IOTC_CHECK, 0)
#define SFC_READ_SR1  _IO(SFC_IOTC_CHECK, 1)
#define SFC_READ_SR2  _IO(SFC_IOTC_CHECK, 2)

#define SFC_WRITE_SR _IO(SFC_IOTC_CHECK, 3)
#define SFC_WRITE_SR1 _IO(SFC_IOTC_CHECK, 4)
#define SFC_WRITE_SR2 _IO(SFC_IOTC_CHECK, 5)

#define SFC_DO_READ _IO(SFC_IOTC_CHECK, 6)

#define SFC_SET_BLOCK_LOCK _IO(SFC_IOTC_CHECK, 7)
#define SFC_READ_BLOCK_LOCK_STATUS _IO(SFC_IOTC_CHECK, 8)

#define INGENIC_SFC_MAJOR 238

void dump_sfc_reg(struct sfc *sfc);
void dump_cdt(struct sfc *sfc);
void dump_desc(struct sfc *sfc, uint32_t desc_num);

void ingenic_sfc_init_setup(struct sfc *sfc);
void *sfc_get_paddr(void *);
int32_t create_sfc_desc(struct sfc_flash *, unsigned char *, size_t);
int sfc_sync_cdt(struct sfc *sfc, struct sfc_cdt_xfer *xfer);
struct sfc *sfc_res_init(struct platform_device *);
void sfc_res_deinit(struct sfc *sfc);
uint32_t sfc_get_sta_rt(struct sfc *);
void write_cdt(struct sfc *sfc, struct sfc_cdt *cdt, uint16_t start_index, uint16_t end_index);

int32_t set_flash_timing(struct sfc *, uint32_t, uint32_t, uint32_t, uint32_t);

int32_t sfc_nor_get_special_ops(struct sfc_flash *);

#endif


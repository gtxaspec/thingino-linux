#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/mtd/partitions.h>
#include "../spinand.h"
#include "../ingenic_sfc_common.h"
#include "nand_common.h"

#define ZB_DEVICES_NUM         2
#define TSETUP		5
#define THOLD		5
#define	TSHSL_R		20
#define	TSHSL_W		20

#define TRD		140
#define TPP		1000
#define TBE		5

static struct ingenic_sfcnand_device *zb_nand;

static struct ingenic_sfcnand_base_param zb_param[ZB_DEVICES_NUM] = {

	[0] = {
		/*ZB35Q01B*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.plane_select = 0,
		.ecc_max = 0x4,
#ifdef CONFIG_SPI_STANDARD_MODE
		.need_quad = 0,
#else
		.need_quad = 1,
#endif
	},
	[1] = {
		/*ZB35Q02B*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 2048,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.plane_select = 0,
		.ecc_max = 0x4,
#ifdef CONFIG_SPI_STANDARD_MODE
		.need_quad = 0,
#else
		.need_quad = 1,
#endif
	},
};

static struct device_id_struct device_id[ZB_DEVICES_NUM] = {
	DEVICE_ID_STRUCT(0xa1, "ZB35Q01B", &zb_param[0]),
	DEVICE_ID_STRUCT(0xa2, "ZB35Q02B", &zb_param[1]),
};


static cdt_params_t *zb_get_cdt_params(struct sfc_flash *flash, uint8_t device_id)
{
	CDT_PARAMS_INIT(zb_nand->cdt_params);

	switch(device_id) {
		case 0xa1:
		case 0xa2:
		    break;
	    default:
		    dev_err(flash->dev, "device_id err, please check your  device id: device_id = 0x%02x\n", device_id);
		    return NULL;
	}

	return &zb_nand->cdt_params;
}


static inline int deal_ecc_status(struct sfc_flash *flash, uint8_t device_id, uint8_t ecc_status)
{
	int ret = 0;

	switch(device_id) {
		case 0xa1:
		case 0xa2:
			switch((ecc_status >> 4) & 0x3) {
			    case 0x02:
				    ret = -EBADMSG;
				    break;
			    default:
				    ret = 0;
			}
			break;
		default:
			dev_err(flash->dev, "device_id err, it maybe don`t support this device, check your device id: device_id = 0x%02x\n", device_id);
			ret = -EIO;   //notice!!!

	}
	return ret;
}

#ifndef CONFIG_ZERATUL
static int __init zb_nand_init(void) {
#else
int zb_nand_init(void) {
#endif

	zb_nand = kzalloc(sizeof(*zb_nand), GFP_KERNEL);
	if(!zb_nand) {
		pr_err("alloc zb_nand struct fail\n");
		return -ENOMEM;
	}

	zb_nand->id_manufactory = 0x5E;
	zb_nand->id_device_list = device_id;
	zb_nand->id_device_count = ZB_DEVICES_NUM;

	zb_nand->ops.get_cdt_params = zb_get_cdt_params;
	zb_nand->ops.deal_ecc_status = deal_ecc_status;

	/* use private get feature interface, please define it in this document */
	zb_nand->ops.get_feature = NULL;

	return ingenic_sfcnand_register(zb_nand);
}

#ifndef CONFIG_ZERATUL
fs_initcall(zb_nand_init);
#endif

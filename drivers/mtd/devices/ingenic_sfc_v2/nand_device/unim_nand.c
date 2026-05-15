#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/mtd/partitions.h>
#include "../spinand.h"
#include "../ingenic_sfc_common.h"
#include "nand_common.h"

#define MICRON_DEVICES_NUM         4
#define TSETUP		5
#define THOLD		5
#define	TSHSL_R		20
#define	TSHSL_W		20

#define TRD		50
#define TPP		400
#define TBE		10

static struct ingenic_sfcnand_device *unim_nand;

static struct ingenic_sfcnand_base_param unim_param[MICRON_DEVICES_NUM] = {

	[0] = {
		/* MT29F2G01A   esmt F50L2G41XA */
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		.flashsize = 2 * 1024 * 64 * 2048,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.plane_select = 1,
		.ecc_max = 0x8,
#ifdef CONFIG_SPI_STANDARD_MODE
		.need_quad = 0,
#else
		.need_quad = 1,
#endif
	},
	[1] = {
		/* UM19A9HISW */
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 32 * 1024,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.plane_select = 1,
		.ecc_max = 0x4,
#ifdef CONFIG_SPI_STANDARD_MODE
		.need_quad = 0,
#else
		.need_quad = 1,
#endif
	},
	[2] = {
		/* UM19A0HISW */
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.plane_select = 1,
		.ecc_max = 0x8,
#ifdef CONFIG_SPI_STANDARD_MODE
		.need_quad = 0,
#else
		.need_quad = 1,
#endif
	},
	[3] = {
		/* UM19A8HISW */
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 16 * 1024,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.plane_select = 1,
		.ecc_max = 0x4,
#ifdef CONFIG_SPI_STANDARD_MODE
		.need_quad = 0,
#else
		.need_quad = 1,
#endif
	},


};

static struct device_id_struct device_id[MICRON_DEVICES_NUM] = {
	DEVICE_ID_STRUCT(0x24, "MT29F2G01A", &unim_param[0]),  //esmt nand F50L2G41XA Same ID
	DEVICE_ID_STRUCT(0x0c, "UM19A9HISW", &unim_param[1]),
	DEVICE_ID_STRUCT(0x14, "UM19A0HISW", &unim_param[2]),
	DEVICE_ID_STRUCT(0xfc, "UM19A8HISW", &unim_param[3]),
};


static cdt_params_t *unim_get_cdt_params(struct sfc_flash *flash, uint8_t device_id)
{
	CDT_PARAMS_INIT(unim_nand->cdt_params);

	switch(device_id) {
	    case 0x14:
	    case 0x24:
	    case 0x0c:
	    case 0xfc:
		    break;
	    default:
		    dev_err(flash->dev, "device_id err, please check your  device id: device_id = 0x%02x\n", device_id);
		    return NULL;
	}

	return &unim_nand->cdt_params;
}


static inline int deal_ecc_status(struct sfc_flash *flash, uint8_t device_id, uint8_t ecc_status)
{
	int ret = 0;

	switch(device_id) {
		case 0x14:
		case 0x24:
			switch((ecc_status >> 4) & 0x7) {
				case 0x2:
					dev_err(flash->dev, "SFC ECC:Bit errors greater than 8 bits detected and not corrected.\n");
					ret = -EBADMSG;
					break;
			    default:
					ret = 0x0;
			}
			break;
		case 0x0c:
		case 0xfc:
			switch((ecc_status >> 4) & 0x3) {
				case 0x2:
					ret = -EBADMSG;
					break;
			    default:
					ret = 0x0;
			}
			break;

		default:
			dev_err(flash->dev, "device_id err, it maybe don`t support this device, check your device id: device_id = 0x%02x\n", device_id);
			ret = -EIO;   //notice!!!
	}
	return ret;
}


#ifndef CONFIG_ZERATUL
static int __init unim_nand_init(void) {
#else
int unim_nand_init(void) {
#endif

	unim_nand = kzalloc(sizeof(*unim_nand), GFP_KERNEL);
	if(!unim_nand) {
		pr_err("alloc unim_nand struct fail\n");
		return -ENOMEM;
	}

	unim_nand->id_manufactory = 0xB0;
	unim_nand->id_device_list = device_id;
	unim_nand->id_device_count = MICRON_DEVICES_NUM;

	unim_nand->ops.get_cdt_params = unim_get_cdt_params;
	unim_nand->ops.deal_ecc_status = deal_ecc_status;

	/* use private get feature interface, please define it in this document */
	unim_nand->ops.get_feature = NULL;

	return ingenic_sfcnand_register(unim_nand);
}

#ifndef CONFIG_ZERATUL
fs_initcall(unim_nand_init);
#endif

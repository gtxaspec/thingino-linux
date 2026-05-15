#ifndef __NAND_COMMON_H
#define __NAND_COMMON_H
#include "../jz_sfc_nand.h"

void nand_pageread_to_cache(struct sfc_transfer *transfer, struct cmd_info *cmd, struct flash_operation_message *op_info);

void nand_single_read(struct sfc_transfer *transfer, struct cmd_info *cmd,
	struct flash_operation_message *op_info, uint8_t columnlen);

void nand_quad_read(struct sfc_transfer *transfer, struct cmd_info *cmd,
	struct flash_operation_message *op_info, uint8_t columnlen);

void nand_write_enable(struct sfc_transfer *transfer, struct cmd_info *cmd, struct flash_operation_message *op_info);


void nand_single_load(struct sfc_transfer *transfer, struct cmd_info *cmd, struct flash_operation_message *op_info);

void nand_quad_load(struct sfc_transfer *transfer, struct cmd_info *cmd, struct flash_operation_message *op_info);

void nand_program_exec(struct sfc_transfer *transfer, struct cmd_info *cmd, struct flash_operation_message *op_info);

int32_t nand_get_program_feature(struct flash_operation_message *op_info);

void nand_block_erase(struct sfc_transfer *transfer, struct cmd_info *cmd, struct flash_operation_message *op_info);

int32_t nand_get_erase_feature(struct flash_operation_message *op_info);

void nand_set_feature(struct sfc_transfer *transfer, struct cmd_info *cmd, uint8_t addr, uint8_t val);

void nand_get_feature(struct sfc_transfer *transfer, struct cmd_info *cmd, uint8_t addr, uint8_t *val);

void winbond_reset(struct sfc_flash *flash);

int32_t nand_flash_init(struct sfc_flash *flash);
int winbond_nand_init(void);
int xtx_mid2c_nand_init(void);
int fs_nand_init(void);
int fm_nand_init(void);
int ds_nand_init(void);
int mxic_nand_init(void);
int ato_nand_init(void);
int xtx_nand_init(void);
int xtx_mid0b_nand_init(void);
int gd_nand_init(void);
int wodposit_nand_init(void);
int as_nand_init(void);
int sky_nand_init(void);

#endif

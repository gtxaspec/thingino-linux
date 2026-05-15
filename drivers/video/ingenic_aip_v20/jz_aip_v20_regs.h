/*
 * Ingenic AIP driver ver2.0
 *
 * Copyright (c) 2023 LiuTianyang
 *
 * This file is released under the GPLv2
 */

#ifndef _JZ_AIP_V20_REGS_H_
#define _JZ_AIP_V20_REGS_H_

#include <linux/bitops.h>

#define AIP_V20_WRITEL(offset, val)           writel(val, aip->io_regs + offset)
#define AIP_V20_READL(offset)                 readl(aip->io_regs + offset)


/*************************************************************
 * AIP F CTL
 *************************************************************/
#define AIP_V20_F_CTL                         (0x0200)

#define AIP_V20_F_CTL_BUSY_WIDTH              (1)
#define AIP_V20_F_CTL_BUSY_SHIFT              (0)
#define AIP_V20_F_CTL_BUSY_MASK               ((1 << AIP_V20_F_CTL_BUSY_WIDTH) - 1)
#define AIP_V20_F_CTL_BUSY_FIELD              (AIP_V20_F_CTL_BUSY_MASK << AIP_V20_F_CTL_BUSY_SHIFT)

#define AIP_V20_F_CTL_SOFT_RESET_WIDTH        (1)
#define AIP_V20_F_CTL_SOFT_RESET_SHIFT        (1)
#define AIP_V20_F_CTL_SOFT_RESET_MASK         ((1 << AIP_V20_F_CTL_SOFT_RESET_WIDTH) - 1)
#define AIP_V20_F_CTL_SOFT_RESET_FIELD        (AIP_V20_F_CTL_SOFT_RESET_MASK << AIP_V20_F_CTL_SOFT_RESET_SHIFT)

#define AIP_V20_F_CTL_CHAIN_BUSY_WIDTH        (1)
#define AIP_V20_F_CTL_CHAIN_BUSY_SHIFT        (2)
#define AIP_V20_F_CTL_CHAIN_BUSY_MASK         ((1 << AIP_V20_F_CTL_CHAIN_BUSY_WIDTH) - 1)
#define AIP_V20_F_CTL_CHAIN_BUSY_FIELD        (AIP_V20_F_CTL_CHAIN_BUSY_MASK << AIP_V20_F_CTL_CHAIN_BUSY_SHIFT)

#define AIP_V20_F_CTL_AXI_EMPTY_WIDTH         (1)
#define AIP_V20_F_CTL_AXI_EMPTY_SHIFT         (3)
#define AIP_V20_F_CTL_AXI_EMPTY_MASK          ((1 << AIP_V20_F_CTL_AXI_EMPTY_WIDTH) - 1)
#define AIP_V20_F_CTL_AXI_EMPTY_FIELD         (AIP_V20_F_CTL_AXI_EMPTY_MASK << AIP_V20_F_CTL_AXI_EMPTY_SHIFT)

#define AIP_V20_F_CTL_DONE_R_WIDTH            (1)
#define AIP_V20_F_CTL_DONE_R_SHIFT            (31)
#define AIP_V20_F_CTL_DONE_R_MASK             ((1 << AIP_V20_F_CTL_DONE_R_WIDTH) - 1)
#define AIP_V20_F_CTL_DONE_R_FIELD            (AIP_V20_F_CTL_DONE_R_MASK << AIP_V20_F_CTL_DONE_R_SHIFT)


/*************************************************************
 * AIP F IRQ
 *************************************************************/
#define AIP_V20_F_IRQ                         (0x0204)

#define AIP_V20_F_IRQ_IRQ_WIDTH               (1)
#define AIP_V20_F_IRQ_IRQ_SHIFT               (0)
#define AIP_V20_F_IRQ_IRQ_MASK                ((1 << AIP_V20_F_IRQ_IRQ_WIDTH) - 1)
#define AIP_V20_F_IRQ_IRQ_FIELD               (AIP_V20_F_IRQ_IRQ_MASK << AIP_V20_F_IRQ_IRQ_SHIFT)

#define AIP_V20_F_IRQ_CHAIN_IRQ_WIDTH         (1)
#define AIP_V20_F_IRQ_CHAIN_IRQ_SHIFT         (1)
#define AIP_V20_F_IRQ_CHAIN_IRQ_MASK          ((1 << AIP_V20_F_IRQ_CHAIN_IRQ_WIDTH) - 1)
#define AIP_V20_F_IRQ_CHAIN_IRQ_FIELD         (AIP_V20_F_IRQ_CHAIN_IRQ_MASK << AIP_V20_F_IRQ_CHAIN_IRQ_SHIFT)

#define AIP_V20_F_IRQ_TIMEOUT_IRQ_WIDTH       (1)
#define AIP_V20_F_IRQ_TIMEOUT_IRQ_SHIFT       (2)
#define AIP_V20_F_IRQ_TIMEOUT_IRQ_MASK        ((1 << AIP_V20_F_IRQ_TIMEOUT_IRQ_WIDTH) - 1)
#define AIP_V20_F_IRQ_TIMEOUT_IRQ_FIELD       (AIP_V20_F_IRQ_TIMEOUT_IRQ_MASK << AIP_V20_F_IRQ_TIMEOUT_IRQ_SHIFT)


/*************************************************************
 * AIP F CFG
 *************************************************************/
#define AIP_V20_F_CFG                         (0x0208)

#define AIP_V20_F_CFG_IBUS_WIDTH              (1)
#define AIP_V20_F_CFG_IBUS_SHIFT              (0)
#define AIP_V20_F_CFG_IBUS_MASK               ((1 << AIP_V20_F_CFG_IBUS_WIDTH) - 1)
#define AIP_V20_F_CFG_IBUS_FIELD              (AIP_V20_F_CFG_IBUS_MASK << AIP_V20_F_CFG_IBUS_SHIFT)

#define AIP_V20_F_CFG_OBUS_WIDTH              (1)
#define AIP_V20_F_CFG_OBUS_SHIFT              (1)
#define AIP_V20_F_CFG_OBUS_MASK               ((1 << AIP_V20_F_CFG_OBUS_WIDTH) - 1)
#define AIP_V20_F_CFG_OBUS_FIELD              (AIP_V20_F_CFG_OBUS_MASK << AIP_V20_F_CFG_OBUS_SHIFT)

#define AIP_V20_F_CFG_CKG_MASK_WIDTH          (1)
#define AIP_V20_F_CFG_CKG_MASK_SHIFT          (2)
#define AIP_V20_F_CFG_CKG_MASK_MASK           ((1 << AIP_V20_F_CFG_CKG_MASK_WIDTH) - 1)
#define AIP_V20_F_CFG_CKG_MASK_FIELD          (AIP_V20_F_CFG_CKG_MASK_MASK << AIP_V20_F_CFG_CKG_MASK_SHIFT)

#define AIP_V20_F_CFG_IRQ_MASK_WIDTH          (1)
#define AIP_V20_F_CFG_IRQ_MASK_SHIFT          (3)
#define AIP_V20_F_CFG_IRQ_MASK_MASK           ((1 << AIP_V20_F_CFG_IRQ_MASK_WIDTH) - 1)
#define AIP_V20_F_CFG_IRQ_MASK_FIELD          (AIP_V20_F_CFG_IRQ_MASK_MASK << AIP_V20_F_CFG_IRQ_MASK_SHIFT)

#define AIP_V20_F_CFG_CHAIN_IRQ_MASK_WIDTH    (1)
#define AIP_V20_F_CFG_CHAIN_IRQ_MASK_SHIFT    (4)
#define AIP_V20_F_CFG_CHAIN_IRQ_MASK_MASK     ((1 << AIP_V20_F_CFG_CHAIN_IRQ_MASK_WIDTH) - 1)
#define AIP_V20_F_CFG_CHAIN_IRQ_MASK_FIELD    (AIP_V20_F_CFG_CHAIN_IRQ_MASK_MASK << AIP_V20_F_CFG_CHAIN_IRQ_MASK_SHIFT)

#define AIP_V20_F_CFG_TIMEOUT_IRQ_MASK_WIDTH  (1)
#define AIP_V20_F_CFG_TIMEOUT_IRQ_MASK_SHIFT  (5)
#define AIP_V20_F_CFG_TIMEOUT_IRQ_MASK_MASK   ((1 << AIP_V20_F_CFG_TIMEOUT_IRQ_MASK_WIDTH) - 1)
#define AIP_V20_F_CFG_TIMEOUT_IRQ_MASK_FIELD  (AIP_V20_F_CFG_TIMEOUT_IRQ_MASK_MASK << AIP_V20_F_CFG_TIMEOUT_IRQ_MASK_SHIFT)

#define AIP_V20_F_CFG_TIMEOUT_ENABLE_WIDTH    (1)
#define AIP_V20_F_CFG_TIMEOUT_ENABLE_SHIFT    (6)
#define AIP_V20_F_CFG_TIMEOUT_ENABLE_MASK     ((1 << AIP_V20_F_CFG_TIMEOUT_ENABLE_WIDTH) - 1)
#define AIP_V20_F_CFG_TIMEOUT_ENABLE_FIELD    (AIP_V20_F_CFG_TIMEOUT_ENABLE_MASK << AIP_V20_F_CFG_TIMEOUT_ENABLE_SHIFT)

#define AIP_V20_F_CFG_BW_WIDTH                (3)
#define AIP_V20_F_CFG_BW_SHIFT                (7)
#define AIP_V20_F_CFG_BW_MASK                 ((1 << AIP_V20_F_CFG_BW_WIDTH) - 1)
#define AIP_V20_F_CFG_BW_FIELD                (AIP_V20_F_CFG_BW_MASK << AIP_V20_F_CFG_BW_SHIFT)

#define AIP_V20_F_CFG_ISUM_CLR_WIDTH          (1)
#define AIP_V20_F_CFG_ISUM_CLR_SHIFT          (10)
#define AIP_V20_F_CFG_ISUM_CLR_MASK           ((1 << AIP_V20_F_CFG_ISUM_CLR_WIDTH) - 1)
#define AIP_V20_F_CFG_ISUM_CLR_FIELD          (AIP_V20_F_CFG_ISUM_CLR_MASK << AIP_V20_F_CFG_ISUM_CLR_SHIFT)

#define AIP_V20_F_CFG_OSUM_CLR_WIDTH          (1)
#define AIP_V20_F_CFG_OSUM_CLR_SHIFT          (11)
#define AIP_V20_F_CFG_OSUM_CLR_MASK           ((1 << AIP_V20_F_CFG_OSUM_CLR_WIDTH) - 1)
#define AIP_V20_F_CFG_OSUM_CLR_FIELD          (AIP_V20_F_CFG_OSUM_CLR_MASK << AIP_V20_F_CFG_OSUM_CLR_SHIFT)


/*************************************************************
 * AIP F TIME CNT
 *************************************************************/
#define AIP_V20_F_TIME_CNT                    (0x020c)


/*************************************************************
 * AIP F SCALEX
 *************************************************************/
#define AIP_V20_F_SCALEX                      (0x0210)


/*************************************************************
 * AIP F SCALEY
 *************************************************************/
#define AIP_V20_F_SCALEY                      (0x0214)


/*************************************************************
 * AIP F TRANSX
 *************************************************************/
#define AIP_V20_F_TRANSX                      (0x0218)


/*************************************************************
 * AIP F TRANST
 *************************************************************/
#define AIP_V20_F_TRANSY                      (0x021c)


/*************************************************************
 * AIP F SRC BASE
 *************************************************************/
#define AIP_V20_F_SRC_BASE                    (0x0220)


/*************************************************************
 * AIP F SRC BASE C
 *************************************************************/
#define AIP_V20_F_SRC_BASE_C                  (0x0224)


/*************************************************************
 * AIP F DST BASE
 *************************************************************/
#define AIP_V20_F_DST_BASE                    (0x0228)


/*************************************************************
 * AIP F DST BASE C
 *************************************************************/
#define AIP_V20_F_DST_BASE_C                  (0x022c)


/*************************************************************
 * AIP F SRC SIZE
 *************************************************************/
#define AIP_V20_F_SRC_SIZE                    (0x0230)


/*************************************************************
 * AIP F DST SIZE
 *************************************************************/
#define AIP_V20_F_DST_SIZE                    (0x0234)


/*************************************************************
 * AIP F STRD
 *************************************************************/
#define AIP_V20_F_STRD                        (0x0238)


/*************************************************************
 * AIP F CHAIN BASE
 *************************************************************/
#define AIP_V20_F_CHAIN_BASE                  (0x023c)


/*************************************************************
 * AIP F CHAIN SIZE
 *************************************************************/
#define AIP_V20_F_CHAIN_SIZE                  (0x0240)



/*************************************************************
 * AIP P CTL
 *************************************************************/
#define AIP_V20_P_CTL                         (0x0300)

#define AIP_V20_P_CTL_BUSY_WIDTH              (1)
#define AIP_V20_P_CTL_BUSY_SHIFT              (0)
#define AIP_V20_P_CTL_BUSY_MASK               ((1 << AIP_V20_P_CTL_BUSY_WIDTH) - 1)
#define AIP_V20_P_CTL_BUSY_FIELD              (AIP_V20_P_CTL_BUSY_MASK << AIP_V20_P_CTL_BUSY_SHIFT)

#define AIP_V20_P_CTL_SOFT_RESET_WIDTH        (1)
#define AIP_V20_P_CTL_SOFT_RESET_SHIFT        (1)
#define AIP_V20_P_CTL_SOFT_RESET_MASK         ((1 << AIP_V20_P_CTL_SOFT_RESET_WIDTH) - 1)
#define AIP_V20_P_CTL_SOFT_RESET_FIELD        (AIP_V20_P_CTL_SOFT_RESET_MASK << AIP_V20_P_CTL_SOFT_RESET_SHIFT)

#define AIP_V20_P_CTL_CHAIN_BUSY_WIDTH        (1)
#define AIP_V20_P_CTL_CHAIN_BUSY_SHIFT        (2)
#define AIP_V20_P_CTL_CHAIN_BUSY_MASK         ((1 << AIP_V20_P_CTL_CHAIN_BUSY_WIDTH) - 1)
#define AIP_V20_P_CTL_CHAIN_BUSY_FIELD        (AIP_V20_P_CTL_CHAIN_BUSY_MASK << AIP_V20_P_CTL_CHAIN_BUSY_SHIFT)

#define AIP_V20_P_CTL_AXI_EMPTY_WIDTH         (1)
#define AIP_V20_P_CTL_AXI_EMPTY_SHIFT         (3)
#define AIP_V20_P_CTL_AXI_EMPTY_MASK          ((1 << AIP_V20_P_CTL_AXI_EMPTY_WIDTH) - 1)
#define AIP_V20_P_CTL_AXI_EMPTY_FIELD         (AIP_V20_P_CTL_AXI_EMPTY_MASK << AIP_V20_P_CTL_AXI_EMPTY_SHIFT)


/*************************************************************
 * AIP P IRQ
 *************************************************************/
#define AIP_V20_P_IRQ                         (0x0304)

#define AIP_V20_P_IRQ_IRQ_WIDTH               (1)
#define AIP_V20_P_IRQ_IRQ_SHIFT               (0)
#define AIP_V20_P_IRQ_IRQ_MASK                ((1 << AIP_V20_P_IRQ_IRQ_WIDTH) - 1)
#define AIP_V20_P_IRQ_IRQ_FIELD               (AIP_V20_P_IRQ_IRQ_MASK << AIP_V20_P_IRQ_IRQ_SHIFT)

#define AIP_V20_P_IRQ_CHAIN_IRQ_WIDTH         (1)
#define AIP_V20_P_IRQ_CHAIN_IRQ_SHIFT         (1)
#define AIP_V20_P_IRQ_CHAIN_IRQ_MASK          ((1 << AIP_V20_P_IRQ_CHAIN_IRQ_WIDTH) - 1)
#define AIP_V20_P_IRQ_CHAIN_IRQ_FIELD         (AIP_V20_P_IRQ_CHAIN_IRQ_MASK << AIP_V20_P_IRQ_CHAIN_IRQ_SHIFT)

#define AIP_V20_P_IRQ_TIMEOUT_IRQ_WIDTH       (1)
#define AIP_V20_P_IRQ_TIMEOUT_IRQ_SHIFT       (2)
#define AIP_V20_P_IRQ_TIMEOUT_IRQ_MASK        ((1 << AIP_V20_P_IRQ_TIMEOUT_IRQ_WIDTH) - 1)
#define AIP_V20_P_IRQ_TIMEOUT_IRQ_FIELD       (AIP_V20_P_IRQ_TIMEOUT_IRQ_MASK << AIP_V20_P_IRQ_TIMEOUT_IRQ_SHIFT)


/*************************************************************
 * AIP P CFG
 *************************************************************/
#define AIP_V20_P_CFG                         (0x0308)

#define AIP_V20_P_CFG_IBUS_WIDTH              (1)
#define AIP_V20_P_CFG_IBUS_SHIFT              (0)
#define AIP_V20_P_CFG_IBUS_MASK               ((1 << AIP_V20_P_CFG_IBUS_WIDTH) - 1)
#define AIP_V20_P_CFG_IBUS_FIELD              (AIP_V20_P_CFG_IBUS_MASK << AIP_V20_P_CFG_IBUS_SHIFT)

#define AIP_V20_P_CFG_OBUS_WIDTH              (1)
#define AIP_V20_P_CFG_OBUS_SHIFT              (1)
#define AIP_V20_P_CFG_OBUS_MASK               ((1 << AIP_V20_P_CFG_OBUS_WIDTH) - 1)
#define AIP_V20_P_CFG_OBUS_FIELD              (AIP_V20_P_CFG_OBUS_MASK << AIP_V20_P_CFG_OBUS_SHIFT)

#define AIP_V20_P_CFG_CKG_MASK_WIDTH          (1)
#define AIP_V20_P_CFG_CKG_MASK_SHIFT          (2)
#define AIP_V20_P_CFG_CKG_MASK_MASK           ((1 << AIP_V20_P_CFG_CKG_MASK_WIDTH) - 1)
#define AIP_V20_P_CFG_CKG_MASK_FIELD          (AIP_V20_P_CFG_CKG_MASK_MASK << AIP_V20_P_CFG_CKG_MASK_SHIFT)

#define AIP_V20_P_CFG_IRQ_MASK_WIDTH          (1)
#define AIP_V20_P_CFG_IRQ_MASK_SHIFT          (3)
#define AIP_V20_P_CFG_IRQ_MASK_MASK           ((1 << AIP_V20_P_CFG_IRQ_MASK_WIDTH) - 1)
#define AIP_V20_P_CFG_IRQ_MASK_FIELD          (AIP_V20_P_CFG_IRQ_MASK_MASK << AIP_V20_P_CFG_IRQ_MASK_SHIFT)

#define AIP_V20_P_CFG_CHAIN_IRQ_MASK_WIDTH    (1)
#define AIP_V20_P_CFG_CHAIN_IRQ_MASK_SHIFT    (4)
#define AIP_V20_P_CFG_CHAIN_IRQ_MASK_MASK     ((1 << AIP_V20_P_CFG_CHAIN_IRQ_MASK_WIDTH) - 1)
#define AIP_V20_P_CFG_CHAIN_IRQ_MASK_FIELD    (AIP_V20_P_CFG_CHAIN_IRQ_MASK_MASK << AIP_V20_P_CFG_CHAIN_IRQ_MASK_SHIFT)

#define AIP_V20_P_CFG_TIMEOUT_IRQ_MASK_WIDTH  (1)
#define AIP_V20_P_CFG_TIMEOUT_IRQ_MASK_SHIFT  (5)
#define AIP_V20_P_CFG_TIMEOUT_IRQ_MASK_MASK   ((1 << AIP_V20_P_CFG_TIMEOUT_IRQ_MASK_WIDTH) - 1)
#define AIP_V20_P_CFG_TIMEOUT_IRQ_MASK_FIELD  (AIP_V20_P_CFG_TIMEOUT_IRQ_MASK_MASK << AIP_V20_P_CFG_TIMEOUT_IRQ_MASK_SHIFT)

#define AIP_V20_P_CFG_TIMEOUT_ENABLE_WIDTH    (1)
#define AIP_V20_P_CFG_TIMEOUT_ENABLE_SHIFT    (6)
#define AIP_V20_P_CFG_TIMEOUT_ENABLE_MASK     ((1 << AIP_V20_P_CFG_TIMEOUT_ENABLE_WIDTH) - 1)
#define AIP_V20_P_CFG_TIMEOUT_ENABLE_FIELD    (AIP_V20_P_CFG_TIMEOUT_ENABLE_MASK << AIP_V20_P_CFG_TIMEOUT_ENABLE_SHIFT)

#define AIP_V20_P_CFG_ISUM_CLR_WIDTH          (1)
#define AIP_V20_P_CFG_ISUM_CLR_SHIFT          (7)
#define AIP_V20_P_CFG_ISUM_CLR_MASK           ((1 << AIP_V20_P_CFG_ISUM_CLR_WIDTH) - 1)
#define AIP_V20_P_CFG_ISUM_CLR_FIELD          (AIP_V20_P_CFG_ISUM_CLR_MASK << AIP_V20_P_CFG_ISUM_CLR_SHIFT)

#define AIP_V20_P_CFG_OSUM_CLR_WIDTH          (1)
#define AIP_V20_P_CFG_OSUM_CLR_SHIFT          (8)
#define AIP_V20_P_CFG_OSUM_CLR_MASK           ((1 << AIP_V20_P_CFG_OSUM_CLR_WIDTH) - 1)
#define AIP_V20_P_CFG_OSUM_CLR_FIELD          (AIP_V20_P_CFG_OSUM_CLR_MASK << AIP_V20_P_CFG_OSUM_CLR_SHIFT)


/*************************************************************
 * AIP P TIME CNT
 *************************************************************/
#define AIP_V20_P_TIME_CNT                    (0x030c)


/*************************************************************
 * AIP P MODE
 *************************************************************/
#define AIP_V20_P_MODE                        (0x0310)

#define AIP_V20_P_MODE_BW_WIDTH               (2)
#define AIP_V20_P_MODE_BW_SHIFT               (0)
#define AIP_V20_P_MODE_BW_MASK                ((1 << AIP_V20_P_CFG_BW_WIDTH) - 1)
#define AIP_V20_P_MODE_BW_FIELD               (AIP_V20_P_MODE_BW_MASK << AIP_V20_P_MODE_BW_SHIFT)


/*************************************************************
 * AIP P SRC YBASE
 *************************************************************/
#define AIP_V20_P_SRC_YBASE                   (0x0314)


/*************************************************************
 * AIP P SRC CBASE
 *************************************************************/
#define AIP_V20_P_SRC_CBASE                   (0x0318)


/*************************************************************
 * AIP P SRC STRIDE
 *************************************************************/
#define AIP_V20_P_SRC_STRIDE                  (0x031c)


/*************************************************************
 * AIP P DST BASE
 *************************************************************/
#define AIP_V20_P_DST_BASE                    (0x0320)


/*************************************************************
 * AIP P DST STRIDE
 *************************************************************/
#define AIP_V20_P_DST_STRIDE                  (0x0324)


/*************************************************************
 * AIP P DWH
 *************************************************************/
#define AIP_V20_P_DWH                         (0x0328)


/*************************************************************
 * AIP P SWH
 *************************************************************/
#define AIP_V20_P_SWH                         (0x032c)


/*************************************************************
 * AIP P DUMMY_VAL
 *************************************************************/
#define AIP_V20_P_DUMMT_VAL                   (0x0330)


/*************************************************************
 * AIP P COEF
 *************************************************************/
#define AIP_V20_P_COEF0                       (0x0334)
#define AIP_V20_P_COEF1                       (0x0338)
#define AIP_V20_P_COEF2                       (0x033c)
#define AIP_V20_P_COEF3                       (0x0340)
#define AIP_V20_P_COEF4                       (0x0344)
#define AIP_V20_P_COEF5                       (0x0348)
#define AIP_V20_P_COEF6                       (0x034c)
#define AIP_V20_P_COEF7                       (0x0350)
#define AIP_V20_P_COEF8                       (0x0354)
#define AIP_V20_P_COEF9                       (0x0358)
#define AIP_V20_P_COEF10                      (0x035c)
#define AIP_V20_P_COEF11                      (0x0360)
#define AIP_V20_P_COEF12                      (0x0364)
#define AIP_V20_P_COEF13                      (0x0368)
#define AIP_V20_P_COEF14                      (0x036c)


/*************************************************************
 * AIP P PARAM
 *************************************************************/
#define AIP_V20_P_PARAM0                      (0x0370)
#define AIP_V20_P_PARAM1                      (0x0374)
#define AIP_V20_P_PARAM2                      (0x0378)
#define AIP_V20_P_PARAM3                      (0x037c)
#define AIP_V20_P_PARAM4                      (0x0380)
#define AIP_V20_P_PARAM5                      (0x0384)
#define AIP_V20_P_PARAM6                      (0x0388)
#define AIP_V20_P_PARAM7                      (0x038c)
#define AIP_V20_P_PARAM8                      (0x0390)


/*************************************************************
 * AIP P OFST
 *************************************************************/
#define AIP_V20_P_OFST                        (0x0394)

#define AIP_V20_P_OFST_NV2BGR_OFSET0_WIDTH    (8)
#define AIP_V20_P_OFST_NV2BGR_OFSET0_SHIFT    (0)
#define AIP_V20_P_OFST_NV2BGR_OFSET0_MASK     ((1 << AIP_V20_P_OFST_NV2BGR_OFSET0_WIDTH) - 1)
#define AIP_V20_P_OFST_NV2BGR_OFSET0_FIELD    (AIP_V20_P_OFST_NV2BGR_OFSET0_MASK << AIP_V20_P_OFST_NV2BGR_OFSET0_SHIFT)

#define AIP_V20_P_OFST_NV2BGR_OFSET1_WIDTH    (8)
#define AIP_V20_P_OFST_NV2BGR_OFSET1_SHIFT    (8)
#define AIP_V20_P_OFST_NV2BGR_OFSET1_MASK     ((1 << AIP_V20_P_OFST_NV2BGR_OFSET1_WIDTH) - 1)
#define AIP_V20_P_OFST_NV2BGR_OFSET1_FIELD    (AIP_V20_P_OFST_NV2BGR_OFSET1_MASK << AIP_V20_P_OFST_NV2BGR_OFSET1_SHIFT)

#define AIP_V20_P_OFST_NV2BGR_ALPHA_WIDTH     (8)
#define AIP_V20_P_OFST_NV2BGR_ALPHA_SHIFT     (16)
#define AIP_V20_P_OFST_NV2BGR_ALPHA_MASK      ((1 << AIP_V20_P_OFST_NV2BGR_ALPHA_WIDTH) - 1)
#define AIP_V20_P_OFST_NV2BGR_ALPHA_FIELD     (AIP_V20_P_OFST_NV2BGR_ALPHA_MASK << AIP_V20_P_OFST_NV2BGR_ALPHA_SHIFT)

#define AIP_V20_P_OFST_NV2BGR_ORDER_WIDTH     (4)
#define AIP_V20_P_OFST_NV2BGR_ORDER_SHIFT     (24)
#define AIP_V20_P_OFST_NV2BGR_ORDER_MASK      ((1 << AIP_V20_P_OFST_NV2BGR_ORDER_WIDTH) - 1)
#define AIP_V20_P_OFST_NV2BGR_ORDER_FIELD     (AIP_V20_P_OFST_NV2BGR_ORDER_MASK << AIP_V20_P_OFST_NV2BGR_ORDER_SHIFT)


/*************************************************************
 * AIP P CHAIN BASE
 *************************************************************/
#define AIP_V20_P_CHAIN_BASE                  (0x0398)


/*************************************************************
 * AIP P CHAIN SIZE
 *************************************************************/
#define AIP_V20_P_CHAIN_SIZE                  (0x039c)


/*************************************************************
 * AIP P ISUM
 *************************************************************/
#define AIP_V20_P_ISUM                        (0x03a0)


/*************************************************************
 * AIP P OSUM
 *************************************************************/
#define AIP_V20_P_OSUM                        (0x03a4)



/*************************************************************
 * AIP T CTL
 *************************************************************/
#define AIP_V20_T_CTL                         (0x0000)

#define AIP_V20_T_CTL_BUSY_WIDTH              (1)
#define AIP_V20_T_CTL_BUSY_SHIFT              (0)
#define AIP_V20_T_CTL_BUSY_MASK               ((1 << AIP_V20_T_CTL_BUSY_WIDTH) - 1)
#define AIP_V20_T_CTL_BUSY_FIELD              (AIP_V20_T_CTL_BUSY_MASK << AIP_V20_T_CTL_BUSY_SHIFT)

#define AIP_V20_T_CTL_SOFT_RESET_WIDTH        (1)
#define AIP_V20_T_CTL_SOFT_RESET_SHIFT        (1)
#define AIP_V20_T_CTL_SOFT_RESET_MASK         ((1 << AIP_V20_T_CTL_SOFT_RESET_WIDTH) - 1)
#define AIP_V20_T_CTL_SOFT_RESET_FIELD        (AIP_V20_T_CTL_SOFT_RESET_MASK << AIP_V20_T_CTL_SOFT_RESET_SHIFT)

#define AIP_V20_T_CTL_CHAIN_BUSY_WIDTH        (1)
#define AIP_V20_T_CTL_CHAIN_BUSY_SHIFT        (2)
#define AIP_V20_T_CTL_CHAIN_BUSY_MASK         ((1 << AIP_V20_T_CTL_CHAIN_BUSY_WIDTH) - 1)
#define AIP_V20_T_CTL_CHAIN_BUSY_FIELD        (AIP_V20_T_CTL_CHAIN_BUSY_MASK << AIP_V20_T_CTL_CHAIN_BUSY_SHIFT)

#define AIP_V20_T_CTL_AXI_EMPTY_WIDTH         (1)
#define AIP_V20_T_CTL_AXI_EMPTY_SHIFT         (3)
#define AIP_V20_T_CTL_AXI_EMPTY_MASK          ((1 << AIP_V20_T_CTL_AXI_EMPTY_WIDTH) - 1)
#define AIP_V20_T_CTL_AXI_EMPTY_FIELD         (AIP_V20_T_CTL_AXI_EMPTY_MASK << AIP_V20_T_CTL_AXI_EMPTY_SHIFT)


#define AIP_V20_T_CTL_TASK_WIDTH              (1)
#define AIP_V20_T_CTL_TASK_SHIFT              (4)
#define AIP_V20_T_CTL_TASK_MASK               ((1 << AIP_V20_T_CTL_TASK_WIDTH) - 1)
#define AIP_V20_T_CTL_TASK_FIELD              (AIP_V20_T_CTL_TASK_MASK << AIP_V20_T_CTL_TASK_SHIFT)


/*************************************************************
 * AIP T IRQ
 *************************************************************/
#define AIP_V20_T_IRQ                         (0x0004)

#define AIP_V20_T_IRQ_IRQ_WIDTH               (1)
#define AIP_V20_T_IRQ_IRQ_SHIFT               (0)
#define AIP_V20_T_IRQ_IRQ_MASK                ((1 << AIP_V20_T_IRQ_IRQ_WIDTH) - 1)
#define AIP_V20_T_IRQ_IRQ_FIELD               (AIP_V20_T_IRQ_IRQ_MASK << AIP_V20_T_IRQ_IRQ_SHIFT)

#define AIP_V20_T_IRQ_CHAIN_IRQ_WIDTH         (1)
#define AIP_V20_T_IRQ_CHAIN_IRQ_SHIFT         (1)
#define AIP_V20_T_IRQ_CHAIN_IRQ_MASK          ((1 << AIP_V20_T_IRQ_CHAIN_IRQ_WIDTH) - 1)
#define AIP_V20_T_IRQ_CHAIN_IRQ_FIELD         (AIP_V20_T_IRQ_CHAIN_IRQ_MASK << AIP_V20_T_IRQ_CHAIN_IRQ_SHIFT)

#define AIP_V20_T_IRQ_TIMEOUT_IRQ_WIDTH       (1)
#define AIP_V20_T_IRQ_TIMEOUT_IRQ_SHIFT       (2)
#define AIP_V20_T_IRQ_TIMEOUT_IRQ_MASK        ((1 << AIP_V20_T_IRQ_TIMEOUT_IRQ_WIDTH) - 1)
#define AIP_V20_T_IRQ_TIMEOUT_IRQ_FIELD       (AIP_V20_T_IRQ_TIMEOUT_IRQ_MASK << AIP_V20_T_IRQ_TIMEOUT_IRQ_SHIFT)

#define AIP_V20_T_IRQ_TASK_WIDTH              (1)
#define AIP_V20_T_IRQ_TASK_SHIFT              (3)
#define AIP_V20_T_IRQ_TASK_MASK               ((1 << AIP_V20_T_IRQ_TASK_WIDTH) - 1)
#define AIP_V20_T_IRQ_TASK_FIELD              (AIP_V20_T_IRQ_TASK_MASK << AIP_V20_T_IRQ_TASK_SHIFT)


/*************************************************************
 * AIP T CFG
 *************************************************************/
#define AIP_V20_T_CFG                         (0x0008)

#define AIP_V20_T_CFG_IBUS_WIDTH              (1)
#define AIP_V20_T_CFG_IBUS_SHIFT              (0)
#define AIP_V20_T_CFG_IBUS_MASK               ((1 << AIP_V20_T_CFG_IBUS_WIDTH) - 1)
#define AIP_V20_T_CFG_IBUS_FIELD              (AIP_V20_T_CFG_IBUS_MASK << AIP_V20_T_CFG_IBUS_SHIFT)

#define AIP_V20_T_CFG_OBUS_WIDTH              (1)
#define AIP_V20_T_CFG_OBUS_SHIFT              (1)
#define AIP_V20_T_CFG_OBUS_MASK               ((1 << AIP_V20_T_CFG_OBUS_WIDTH) - 1)
#define AIP_V20_T_CFG_OBUS_FIELD              (AIP_V20_T_CFG_OBUS_MASK << AIP_V20_T_CFG_OBUS_SHIFT)

#define AIP_V20_T_CFG_CKG_MASK_WIDTH          (1)
#define AIP_V20_T_CFG_CKG_MASK_SHIFT          (2)
#define AIP_V20_T_CFG_CKG_MASK_MASK           ((1 << AIP_V20_T_CFG_CKG_MASK_WIDTH) - 1)
#define AIP_V20_T_CFG_CKG_MASK_FIELD          (AIP_V20_T_CFG_CKG_MASK_MASK << AIP_V20_T_CFG_CKG_MASK_SHIFT)

#define AIP_V20_T_CFG_IRQ_MASK_WIDTH          (1)
#define AIP_V20_T_CFG_IRQ_MASK_SHIFT          (3)
#define AIP_V20_T_CFG_IRQ_MASK_MASK           ((1 << AIP_V20_T_CFG_IRQ_MASK_WIDTH) - 1)
#define AIP_V20_T_CFG_IRQ_MASK_FIELD          (AIP_V20_T_CFG_IRQ_MASK_MASK << AIP_V20_T_CFG_IRQ_MASK_SHIFT)

#define AIP_V20_T_CFG_CHAIN_IRQ_MASK_WIDTH    (1)
#define AIP_V20_T_CFG_CHAIN_IRQ_MASK_SHIFT    (4)
#define AIP_V20_T_CFG_CHAIN_IRQ_MASK_MASK     ((1 << AIP_V20_T_CFG_CHAIN_IRQ_MASK_WIDTH) - 1)
#define AIP_V20_T_CFG_CHAIN_IRQ_MASK_FIELD    (AIP_V20_T_CFG_CHAIN_IRQ_MASK_MASK << AIP_V20_T_CFG_CHAIN_IRQ_MASK_SHIFT)

#define AIP_V20_T_CFG_TIMEOUT_IRQ_MASK_WIDTH  (1)
#define AIP_V20_T_CFG_TIMEOUT_IRQ_MASK_SHIFT  (5)
#define AIP_V20_T_CFG_TIMEOUT_IRQ_MASK_MASK   ((1 << AIP_V20_T_CFG_TIMEOUT_IRQ_MASK_WIDTH) - 1)
#define AIP_V20_T_CFG_TIMEOUT_IRQ_MASK_FIELD  (AIP_V20_T_CFG_TIMEOUT_IRQ_MASK_MASK << AIP_V20_T_CFG_TIMEOUT_IRQ_MASK_SHIFT)

#define AIP_V20_T_CFG_TIMEOUT_ENABLE_WIDTH    (1)
#define AIP_V20_T_CFG_TIMEOUT_ENABLE_SHIFT    (6)
#define AIP_V20_T_CFG_TIMEOUT_ENABLE_MASK     ((1 << AIP_V20_T_CFG_TIMEOUT_ENABLE_WIDTH) - 1)
#define AIP_V20_T_CFG_TIMEOUT_ENABLE_FIELD    (AIP_V20_T_CFG_TIMEOUT_ENABLE_MASK << AIP_V20_T_CFG_TIMEOUT_ENABLE_SHIFT)

#define AIP_V20_T_CFG_TASK_IRQ_WIDTH          (1)
#define AIP_V20_T_CFG_TASK_IRQ_SHIFT          (7)
#define AIP_V20_T_CFG_TASK_IRQ_MASK           ((1 << AIP_V20_T_CFG_TASK_IRQ_WIDTH) - 1)
#define AIP_V20_T_CFG_TASK_IRQ_FIELD          (AIP_V20_T_CFG_TASK_IRQ_MASK << AIP_V20_T_CFG_TASK_IRQ_SHIFT)

#define AIP_V20_T_CFG_ISUM_CLR_WIDTH          (1)
#define AIP_V20_T_CFG_ISUM_CLR_SHIFT          (8)
#define AIP_V20_T_CFG_ISUM_CLR_MASK           ((1 << AIP_V20_T_CFG_ISUM_CLR_WIDTH) - 1)
#define AIP_V20_T_CFG_ISUM_CLR_FIELD          (AIP_V20_T_CFG_ISUM_CLR_MASK << AIP_V20_T_CFG_ISUM_CLR_SHIFT)

#define AIP_V20_T_CFG_OSUM_CLR_WIDTH          (1)
#define AIP_V20_T_CFG_OSUM_CLR_SHIFT          (9)
#define AIP_V20_T_CFG_OSUM_CLR_MASK           ((1 << AIP_V20_T_CFG_OSUM_CLR_WIDTH) - 1)
#define AIP_V20_T_CFG_OSUM_CLR_FIELD          (AIP_V20_T_CFG_OSUM_CLR_MASK << AIP_V20_T_CFG_OSUM_CLR_SHIFT)

#define AIP_V20_T_CFG_IRQ_SELECT_WIDTH        (1)
#define AIP_V20_T_CFG_IRQ_SELECT_SHIFT        (10)
#define AIP_V20_T_CFG_IRQ_SELECT_MASK         ((1 << AIP_V20_T_CFG_IRQ_SELECT_WIDTH) - 1)
#define AIP_V20_T_CFG_IRQ_SELECT_FIELD        (AIP_V20_T_CFG_IRQ_SELECT_MASK << AIP_V20_T_CFG_IRQ_SELECT_SHIFT)


/*************************************************************
 * AIP T TIMEOUT
 *************************************************************/
#define AIP_V20_T_TIMEOUT                     (0x000c)


/*************************************************************
 * AIP T TASK LEN
 *************************************************************/
#define AIP_V20_T_TASK_LEN                    (0x0010)

#define AIP_V20_T_TASK_LEN_RES1_WIDTH         (1)
#define AIP_V20_T_TASK_LEN_RES1_SHIFT         (0)
#define AIP_V20_T_TASK_LEN_RES1_MASK          ((1 << AIP_V20_T_TASK_LEN_RES1_WIDTH) - 1)
#define AIP_V20_T_TASK_LEN_RES1_FIELD         (AIP_V20_T_TASK_LEN_RES1_MASK << AIP_V20_T_TASK_LEN_RES2_SHIFT)

#define AIP_V20_T_TASK_LEN_VALUE_WIDTH        (15)
#define AIP_V20_T_TASK_LEN_VALUE_SHIFT        (1)
#define AIP_V20_T_TASK_LEN_VALUE_MASK         ((1 << AIP_V20_T_TASK_LEN_VALUE_WIDTH) - 1)
#define AIP_V20_T_TASK_LEN_VALUE_FIELD        (AIP_V20_T_TASK_LEN_VALUE_MASK << AIP_V20_T_TASK_LEN_VALUE_SHIFT)

#define AIP_V20_T_TASK_LEN_RES2_WIDTH         (1)
#define AIP_V20_T_TASK_LEN_RES2_SHIFT         (16)
#define AIP_V20_T_TASK_LEN_RES2_MASK          ((1 << AIP_V20_T_TASK_LEN_RES2_WIDTH) - 1)
#define AIP_V20_T_TASK_LEN_RES2_FIELD         (AIP_V20_T_TASK_LEN_RES2_MASK << AIP_V20_T_TASK_LEN_RES2_SHIFT)

#define AIP_V20_T_TASK_LEN_SKIP_WIDTH         (15)
#define AIP_V20_T_TASK_LEN_SKIP_SHIFT         (17)
#define AIP_V20_T_TASK_LEN_SKIP_MASK          ((1 << AIP_V20_T_TASK_LEN_SKIP_WIDTH) - 1)
#define AIP_V20_T_TASK_LEN_SKIP_FIELD         (AIP_V20_T_TASK_LEN_SKIP_MASK << AIP_V20_T_TASK_LEN_SKIP_SHIFT)


/*************************************************************
 * AIP T YBASE SRC
 *************************************************************/
#define AIP_V20_T_YBASE_SRC                   (0x0014)


/*************************************************************
 * AIP T CBASE SRC
 *************************************************************/
#define AIP_V20_T_CBASE_SRC                   (0x0018)


/*************************************************************
 * AIP T BASE DST
 *************************************************************/
#define AIP_V20_T_BASE_DST                    (0x001c)


/*************************************************************
 * AIP T STRIDE
 *************************************************************/
#define AIP_V20_T_STRIDE                      (0x0020)


/*************************************************************
 * AIP T WH
 *************************************************************/
#define AIP_V20_T_WH                          (0x0024)


/*************************************************************
 * AIP T PARAM
 *************************************************************/
#define AIP_V20_T_PARAM0                      (0x0028)
#define AIP_V20_T_PARAM1                      (0x002c)
#define AIP_V20_T_PARAM2                      (0x0030)
#define AIP_V20_T_PARAM3                      (0x0034)
#define AIP_V20_T_PARAM4                      (0x0038)
#define AIP_V20_T_PARAM5                      (0x003c)
#define AIP_V20_T_PARAM6                      (0x0040)
#define AIP_V20_T_PARAM7                      (0x0044)
#define AIP_V20_T_PARAM8                      (0x0048)


/*************************************************************
 * AIP T OFST
 *************************************************************/
#define AIP_V20_T_OFST                        (0x004c)

#define AIP_V20_T_OFST_NV2BGR_OFSET0_WIDTH    (8)
#define AIP_V20_T_OFST_NV2BGR_OFSET0_SHIFT    (0)
#define AIP_V20_T_OFST_NV2BGR_OFSET0_MASK     ((1 << AIP_V20_T_OFST_NV2BGR_OFSET0_WIDTH) - 1)
#define AIP_V20_T_OFST_NV2BGR_OFSET0_FIELD    (AIP_V20_T_OFST_NV2BGR_OFSET0_MASK << AIP_V20_T_OFST_NV2BGR_OFSET0_SHIFT)

#define AIP_V20_T_OFST_NV2BGR_OFSET1_WIDTH    (8)
#define AIP_V20_T_OFST_NV2BGR_OFSET1_SHIFT    (8)
#define AIP_V20_T_OFST_NV2BGR_OFSET1_MASK     ((1 << AIP_V20_T_OFST_NV2BGR_OFSET1_WIDTH) - 1)
#define AIP_V20_T_OFST_NV2BGR_OFSET1_FIELD    (AIP_V20_T_OFST_NV2BGR_OFSET1_MASK << AIP_V20_T_OFST_NV2BGR_OFSET1_SHIFT)

#define AIP_V20_T_OFST_NV2BGR_ALPHA_WIDTH     (8)
#define AIP_V20_T_OFST_NV2BGR_ALPHA_SHIFT     (16)
#define AIP_V20_T_OFST_NV2BGR_ALPHA_MASK      ((1 << AIP_V20_T_OFST_NV2BGR_ALPHA_WIDTH) - 1)
#define AIP_V20_T_OFST_NV2BGR_ALPHA_FIELD     (AIP_V20_T_OFST_NV2BGR_ALPHA_MASK << AIP_V20_T_OFST_NV2BGR_ALPHA_SHIFT)

#define AIP_V20_T_OFST_NV2BGR_ORDER_WIDTH     (4)
#define AIP_V20_T_OFST_NV2BGR_ORDER_SHIFT     (24)
#define AIP_V20_T_OFST_NV2BGR_ORDER_MASK      ((1 << AIP_V20_T_OFST_NV2BGR_ORDER_WIDTH) - 1)
#define AIP_V20_T_OFST_NV2BGR_ORDER_FIELD     (AIP_V20_T_OFST_NV2BGR_ORDER_MASK << AIP_V20_T_OFST_NV2BGR_ORDER_SHIFT)


/*************************************************************
 * AIP T CHAIN BASE
 *************************************************************/
#define AIP_V20_T_CHAIN_BASE                  (0x0050)


/*************************************************************
 * AIP T CHAIN SIZE
 *************************************************************/
#define AIP_V20_T_CHAIN_SIZE                  (0x0054)


/*************************************************************
 * AIP T TASK_SKIP_BASE
 *************************************************************/
#define AIP_V20_T_TASK_SKIP_BASE              (0x0058)


/*************************************************************
 * AIP T ISUM
 *************************************************************/
#define AIP_V20_T_ISUM                        (0x005c)


/*************************************************************
 * AIP T OSUM
 *************************************************************/
#define AIP_V20_T_OSUM                        (0x0060)


#endif

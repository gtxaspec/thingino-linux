/****************************************************************
 *****************************************************************/
#ifndef __HELIX_H__
#define __HELIX_H__

/****************************************************************
  VPU register map
 *****************************************************************/
#define JPEG_BASE                0x132F0000  /* jpeg register base */
#define JPEG_LEN                 0x10000     /* jpeg register len */

/****************
 * REGISTER MAP
 ****************/
#define JPEG_TRIG                0x0       /* trigger register */
#define JPEG_CTRL                0x4       /* global control register */
#define JPEG_NMCU                0x8       /* number of mcu(yuv420: 16 * 16, yuv422: 16 * 32, y: 16 * 16). for example, nmcu = (1920 * 1080) / (16 * 32) */
#define JPEG_FRM_INFO            0xc       /* jpeg width and height */
#define JPEG_LINE_STRD           0x10      /* one line stride (for example 1080P, yuv420: 1920, yuv422: 1920 * 2) */
#define JPEG_RAWY_ADDR           0x14      /* (256byte align) Y pixel start addr (buffer0 addr) */
#define JPEG_RAWC_ADDR           0x18      /* (256byte align) UV pixel start addr (yuy2 is useless)(buffer0 + UV offset) */
#define JPEG_BS_ADDR             0x1C
#define JPEG_CODE_PIX_TOTAL      0x20      /* pixel total number(width/height must 16 align) */
#define JPEG_MARKERS             0x28      /* restart markers function */
#define JPEG_LIMIT_BS_LEN_VAL    0x2c
#define JPEG_VERSION_NUM         0x200     /* jpeg version */
#define JPEG_GLB_STAT            0x100     /* DEBUG: global status register */
#define JPEG_GLB_BS_LEN          0x104     /* DEBUG */
#define JPEG_INTR_STA            0x110     /* interrupt status */
#define JPEG_INTR_CLR            0x114     /* interrupt clear */

#define JPEG_QT_BASE_TAB         0x300     /* 0x300~0x37c, 0x300~0x33c, y base table, 0x340~0x37c, uv base table, base qt of hardware mode */
#define JPEG_QT_TAB              0x400     /* 0x400~0x9fc, constant of 3 qts, every qt contains Y qt and UV qt */
#define JPEG_HUB_TAB             0xa00     /* 0xa00~0x106c, constant of 1 huffman tables which contains dc and ac */

/* JPEG_TRIG */
#define JPEG_START               0         //start encoder one frame(only used in manual_mode)
#define JPEG_SEQ_RESET           1         //reset all registers, only used in initial or uninitial
#define JPEG_FRM_RESET           2         //reset frame level registers after frame down(only used in manual reset mode)
#define JPEG_SLV_RESET           3         //reset slv register (frame H/W,raw addr....)
#define JPEG_RESET_CYCLE         4         //clock cycle of keeping status reset
#define JPEG_FSM_RESET           8         //reset status register, use it in necessary condition
#define RESET_END                31

/* JPEG_CTRL */
#define CTRL_HD_Y                0         //huffman DC table number for Y (default: 0)
#define CTRL_HA_Y                1         //huffman AC table number for Y (default: 0)
#define CTRL_QT_Y                2         //quant table number for Y
#define CTRL_HD_U                4         //huffman DC table number for U (default: 1)
#define CTRL_HA_U                5         //huffman AC table number for U (default: 1)
#define CTRL_QT_U                6         //quant table number for U (Y/U/V has same qt index)
#define CTRL_HD_V                8         //huffman DC table number for V (default: 1)
#define CTRL_HA_V                9         //huffman AC table number for V (default: 1)
#define CTRL_QT_V                10        //quant table number for U (Y/U/V has same qt index)
#define AWLEN                    12        //bs write out AXI bus awlen (default: 0)
#define LIMIT_BS_LEN_EN          16        //limit bs len enable (default: 0)
#define TIMEOUT_EN               17
#define CTRL_QP                  18        //only used hardware generate mode, qt index, range is 0~98

/* JPEG INTERRUPT EN*/
#define JPEG_FRAME_END_INTR_EN   25        //jpeg encode normal end interrupt
#define JPEG_FRAME_END_BSLESS_INTR_EN 26   //jpeg encode normal end interrupt (open bs limit)
#define IVDC_STOP_REQ_INTR_EN    27        //jpeg ivdc stop req interrupt
#define JPEG_TIMEOUT_INTR_EN     28        //jpeg timeout interrupt
#define GRAY_EN                  31        //only encode grey

/* JPEG_NMCU */
#define FRM_MCU_NUMBER           0         //number of mcu(yuv420: 16 * 16, yuv422: 16 * 8, y: 16 * 16). for example, nmcu = (1920 * 1088) / (16 * 8) - 1

/* JPEG_FRM_INFO */
#define FRM_WIDTH                0         //origin width
#define FRM_HEIGHT               16        //origin height

/* JPEG_LINE_STRD */
#define LINE_STRD                0         //yuv422(width * 2), yuv420/y(width), width must 16-align

/* JPEG MARKERS */
#define MARKERS_ENABLE           0         //enable restart marker function
#define MARKERS_NUMBER           16        //markers MCU number reduce 1

/* JPEG LIMIT BS LEN VAL */
#define LIMIT_BS_LEN_VAL         0

/* JPEG_GLB_STAT */
#define BS_LEN                   0         //size of one encoded frame, the unit is byte
#define BUS_FREE                 29        //indicate axi bus free, use with TRIG_BUS_STOP
#define FRM_END                  31        //current frame encoder end

/* JPEG_GLB_FSM */
#define CTRL_FSM                 0         //TODO:
#define RESET_FSM                4         //TODO:
#define SLV_FSM0                 8         //TODO:
#define SLV_FSM1                 12        //TODO:

/* JPEG INTERRUPT */
#define JPEG_FRAME_END_INTR   0        //jpeg encode normal end interrupt
#define JPEG_FRAME_END_BSLESS_INTR 1   //jpeg encode normal end interrupt (open bs limit)
#define IVDC_STOP_REQ_INTR    2        //jpeg ivdc stop req interrupt
#define JPEG_TIMEOUT_INTR     3        //jpeg timeout interrupt

#define HUFFENC_LEN             (412)
#define CPM_HELIX_SR(ID)		(31)
#define CPM_HELIX_STP(ID)		(27)
#define CPM_HELIX_ACK(ID)		(26)

#endif /*__HELIX_H__*/

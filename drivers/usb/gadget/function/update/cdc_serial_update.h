#ifndef __CDC_SERIAL_UPDATE1_H
#define __CDC_SERIAL_UPDATE1_H

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/completion.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/reboot.h>

#define CDC_CONNET_INDEX           254 //脏信息的提示
#define MD5SUM_LENGTH              16
#define UPDATE_MODE                7
#define JZ_HEAD_LENGTH             3
#define JZ_HEAD_FRAME_CHECK_NODE   3
#define JZ_JUDGE                   2
#define JZ_FRAME_LENGTH_OFFSET     4
#define JZ_FRAME_DATA_OFFSET       6
#define JZ_DATA_FRAME_HEDA_LEN     10
#define JZ_FRAME_NUMBER_OFFSET     3
#define FLASH_BURN_FIRMWARE_FLAG   0
#define FLASH_UPDATE_RESET_FLAG    1
#define FLASH_UPDATE_SET_FLAG      2
#define JZ_FRAME1_PACK_INFO_LENGTH	sizeof(jz_frame1_update_pack_info_t)

enum jz_update_frame_number
{
	JZ_UPDATE_SOF_FRAME = 1,
	JZ_UPDATE_ACK_FRAME,
	JZ_UPDATE_DATA_FRAME,
	JZ_UPDATE_ERR_FRAME,
	JZ_UPDATE_EOF_FRAME
};

typedef struct _jz_update_pack_info {
	u16 subpack_size;         //子包大小
	u32 firmware_size;        //固件包大小
	u16 subpackcnt_atonce;    //一次发送子包数量
	u16 firmware_version;     //固件版本
} __attribute__((packed)) jz_frame1_update_pack_info_t;

typedef struct _jz_update_frame_info {
	u16 frame_length;
	u8  cs;
	u8  frame_number;               //Frame号
	u32 frame_num_of_start_send;    //发送第几个子包
	u32 data_frame_index;           //子包序列号
	u32 current_index;              //累加的成功接受的子包数
	u32 batch_index;                //一组包里面的计数
	u32 memcpy_count;               //拷贝固件计数
} jz_update_frame_info_t;

typedef struct _jz_update_firmware_info {
	u8 *firmware_data_buf;        //固件的buf
	u8 *batch_pack_data_buf;      //一组包的buf
	u8 *frame_data_buf;           //一个Frame3的buf
	struct completion malloc_block;
	struct completion data_block;
	struct completion reset_block;
	u32 img_offset;
	u32 img_size;
	u32 frame3_size;              //Frame3大小
	u32 batch_pack_size;          //一组包大小
	u32 last_batch_pack_size;     //最后一组包大小
	u32 batch_pack_cnt;           //发送一组包的次数
} jz_update_firmware_info_t;

typedef struct _jz_firmware_head {
	u32 version;
	u32 img_offset;
	u32 img_size;
	u8 md5[16];
	u8 priv[100];
} jz_firmware_head_t;

#endif

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

#include "u_serial_update.h"
#include "usb_update.h"


#define HEAD_LENGTH			6
#define MD5SUM_LENGTH			16

#define MODEL_OFFSET 			5
#define FRAME_NUMBER_OFFSET 		6
#define FRAME_LENGTH_OFFSET 		7
#define FRAME_DATA_OFFSET 		9
#define DATA_FRAME_HEDA_LEN 		13
#define NOT_ACCEPT_PACKETS		255

#define UPDATE_MODE			7

#define LOG_MODE			8
#define LOG_DATA_SIZE			1024*1024
#define LOG_FRAME_SIZE			498

enum update_frame_number
{
	UPDATE_START_FRAME = 1,
	UPDATE_INTERVAL_FRAME,
	UPDATE_DATA_FRAME,
	UPDATE_DATA_VALI_FRAME,
	UPDATE_FINISH_FRAME,
	UPDATE_UPGRADE_VALI_FRAME,
	UPDATE_FRAME_TYPE_NUM
};

enum log_frame_number
{
	LOG_START_FRAME = 1,
	LOG_INFO_FRAME,
	LOG_DATA_FRAME,
	LOG_FINISH_FRAME,
	LOG_FRAME_TYPE_NUM
};

typedef struct __frame_log2_info {
	u8 FLog2UploadStatus;
	u16 FLog2FrameSize;
	u32 FLog2FileSize;
	u32 FLog2FrameTotalCount;
	u16 FLog2FrameQuantityAtOnce;
	unsigned char FLog2FileHashCode[16];
}__attribute__((packed)) frame_log2_info;

typedef struct __frame_log3_info {
	u32 FLog3FrameIndex;
	u8  FLog3FrameData[LOG_FRAME_SIZE];
}__attribute__((packed)) frame_log3_info;

typedef struct _update_pack_info {
	u8  EnableOTA;
	u8  TimeoutTime;
	u16 PFrameSize;
	u32 PkgSize;
	u32 FrameTotalCount;
	u16 PkgVersion;
	u16 FrameQuantityAtOnce;
} update_pack_info;

typedef struct _update_frame_info {
	u16 FrameLength;
	u8  CS;
	u8  FrameNumber;
	u32 FrameOfStar;
	u32 DataFrameIndex;
	u32 CurrentIndex;
	u32 BatchIndex;
	u32 Memcpy_count;
} update_frame_info;

typedef struct _update_frame6_info {
	u8  F6UpgradeStatus;
	u8  F6UpgradeSubStatus;
}__attribute__((packed)) update_frame6_info;

typedef struct _f_ak_update {
	u8 *firmware_data_buf;
	u8 *subpack_data_buf;
	u8 *frame_data_buf;
	struct completion  malloc_block;
	struct completion  data_block;
	struct completion  reset_block;
	u32 img_offset;
	u32 img_size;
	u32 frame_size;
	u32 subpack_size;
	u32 last_subpack_size;
	u32 subpack_num;
} f_ak_update;


typedef struct _firmware_head {
	u32 version;
	u32 img_offset;
	u32 img_size;
	u8 md5[16];
	u8 priv[100];
} firmware_head_t;


#define PACK_INFO_LENGTH 		sizeof(update_pack_info)

static int protocol_mode;
static int subpack_copy;
static u32 datasum = 1;
update_frame_info frame_info;
f_ak_update ak_update;
update_pack_info pack_info;
static u16 VerifyStatus;

struct task_struct *log_process_thread;
struct task_struct *guard_thread;

static int receive_data_check(u8 *buf, u8 index)
{
	int i;
	u8 receive_data_head[HEAD_LENGTH] = {0x08, 0xEE, 0x00, 0x00, 0x00, 0x07};
	u8 data_head[HEAD_LENGTH];
	u8 *data_buf;
	u8 crc = 0;
	data_buf = buf;


	frame_info.CS = data_buf[frame_info.FrameLength - 1];

	for(i = 0; i < frame_info.FrameLength - 1; i++){
		crc += data_buf[i];
	}

	if(crc != frame_info.CS){
		printk("#########receive frame=%d CS is error(%x),%x########\n", index, crc, frame_info.CS);
		return -1;
	}

	if(protocol_mode == LOG_MODE)
		receive_data_head[MODEL_OFFSET] = LOG_MODE;

	memcpy(data_head, buf, HEAD_LENGTH);
	for(i = 0; i < HEAD_LENGTH ; i++)
	{
		if(data_head[i] != receive_data_head[i]){
			printk("receive frame=%d, data[%d] head is error(%d)\n", index, i, data_head[i]);
			return -1;
		}
	}
	return 0;

};

static int send_frame(u8 index, void *data, int data_len)
{
	u8 send_data_buf[512];
	u16 length = 0;
	u8  crc = 0;
	int i, count;
	u8 send_data_head[HEAD_LENGTH] = {0x09, 0xff, 0x00, 0x00, 0x01, 0x07};

	if(protocol_mode == LOG_MODE)
		send_data_head[MODEL_OFFSET] = LOG_MODE;

	memcpy((void *)send_data_buf, send_data_head, HEAD_LENGTH);
	send_data_buf[FRAME_NUMBER_OFFSET] = index;
	length = HEAD_LENGTH + sizeof(index) + sizeof(length) + data_len + sizeof(crc);
	memcpy((void *)send_data_buf + FRAME_LENGTH_OFFSET,  (void *)&length, sizeof(length));
	if(data != NULL)
		memcpy(send_data_buf + FRAME_DATA_OFFSET, data, data_len);

	send_data_buf[length - 1] = 0;
	for(i = 0; i < length -1; i++)
		crc += send_data_buf[i];
	send_data_buf[length - 1] = crc;
	count = gs_write_cdc(send_data_buf, length);
	if(count != length){
		printk("send frame data is failed,count=%d,length=%d\n",count,length);
		return -1;
	}
	return 0;

};

static void norflash_oprate(int write_flag, int reset_flag)
{
	int flash_flag = 0;
	/*写入升级固件*/
	if (write_flag == 0){
		direct_erase_norflash(ak_update.img_offset, ak_update.img_size);
		direct_write_norflash(ak_update.firmware_data_buf + HEAD_INFO_RESERVED, ak_update.img_offset, ak_update.img_size);
	}

	/*重置flash中的标志位*/
	flash_flag = NORFALSH_RESET_SIGNATURE;
	direct_erase_norflash(NORFLASH_OFFSET, sizeof(int));
	direct_write_norflash(&flash_flag, NORFLASH_OFFSET, sizeof(int));
	if(reset_flag == 0)
		machine_restart("Restart the system after the upgrade is complete");

};

static int get_firmware_head_info(void *buf)
{
	int i;
	firmware_head_t head;
	unsigned char buf_md5sum[16];

	memcpy((void *)&head, buf, sizeof(firmware_head_t));
	ak_update.img_offset = head.img_offset;
	ak_update.img_size   = head.img_size;

	buffer_md5sum_0((char *)ak_update.firmware_data_buf + HEAD_INFO_RESERVED, buf_md5sum, ak_update.img_size);

	for(i = 0; i < MD5SUM_LENGTH; i++){
		if(buf_md5sum[i] != head.md5[i]){
			printk("firmware md5 check is error,buf_md5sum[15]=%d, head.md5[15]=%d\n", buf_md5sum[15], head.md5[16]);
			return -1;
		}
	}
	return 0;

}

static int log_frame_process_thread(void *data)
{
	int ret = -1, flag = 1;
	u8 *log_data_buf = NULL;
	frame_log2_info framelog2;
	frame_log3_info framelog3;
	u32 copy_len, total_len = 0, last_len = 0;


	log_data_buf = kmalloc(LOG_DATA_SIZE, GFP_KERNEL);
	if(log_data_buf == NULL){
		printk("kmalloc cdc data buf is failed\n");
		goto error;
	}
	direct_read_norflash(log_data_buf, LOGDATA_OFFSET, LOG_DATA_SIZE);

	framelog2.FLog2UploadStatus = 0;
	framelog2.FLog2FrameSize = LOG_FRAME_SIZE;
	framelog2.FLog2FileSize = LOG_DATA_SIZE;
	framelog2.FLog2FrameTotalCount = framelog2.FLog2FileSize / framelog2.FLog2FrameSize;
	if(framelog2.FLog2FileSize % framelog2.FLog2FrameSize != 0)
		framelog2.FLog2FrameTotalCount += 1;
	framelog2.FLog2FrameQuantityAtOnce = framelog2.FLog2FrameTotalCount;
	/*上位机一直收不到最后一帧，设备这边做特殊处理	*/
	last_len = framelog2.FLog2FileSize % framelog2.FLog2FrameSize;
	buffer_md5sum_0((char *)log_data_buf ,framelog2.FLog2FileHashCode, framelog2.FLog2FileSize - last_len);
	printk("#########filesize=%d, framesize=%d, frametotalcount=%d##########\n",framelog2.FLog2FrameSize, framelog2.FLog2FileSize, framelog2.FLog2FrameTotalCount);

	ret = send_frame(LOG_INFO_FRAME, (void *)&framelog2, sizeof(frame_log2_info));
	if(ret < 0){
		printk("########send frame failed,mode=%d,frame=%d########",protocol_mode, LOG_INFO_FRAME);
		goto error;
	}

	framelog3.FLog3FrameIndex = 0;
	copy_len = framelog2.FLog2FrameSize;
	while(flag){
		if(framelog3.FLog3FrameIndex == framelog2.FLog2FrameTotalCount -1){
			copy_len = last_len;
			flag = 0;
			printk("###########send log data is end, frame_index=%d,copy_len=%d########\n",framelog3.FLog3FrameIndex,copy_len);
		}

		memcpy(framelog3.FLog3FrameData, log_data_buf + total_len, copy_len);
		send_frame(LOG_DATA_FRAME, (void *)&framelog3, copy_len + sizeof(framelog3.FLog3FrameIndex));
		if(ret < 0){
			msleep(10);
			ret = send_frame(LOG_DATA_FRAME, (void *)&framelog3, sizeof(frame_log3_info));
			if(ret < 0){
				printk("send frame failed,mode=%d,frame=%d",protocol_mode, LOG_DATA_FRAME);
				goto error;
			}
			ret = send_frame(LOG_DATA_FRAME, (void *)&framelog3, sizeof(frame_log3_info));
			if(ret < 0){
				printk("send frame failed,mode=%d,frame=%d",protocol_mode, LOG_DATA_FRAME);
				goto error;
			}
		}
		msleep(1);
		total_len += copy_len;
		framelog3.FLog3FrameIndex += 1;

	}
	return 0;
error:
	complete(&ak_update.reset_block);
	return -1;
}


static int update_start_frame_process(void *buf, int index)
{
	int ret = -1;
	update_frame6_info frame6_info;
	u8	pack_info_data[PACK_INFO_LENGTH];
	u8	*data_buf = NULL;
	data_buf = (u8 *)buf;

	if(VerifyStatus == 1){
		frame6_info.F6UpgradeStatus = NOT_ACCEPT_PACKETS;
		/*反馈给上位机的进度值，随便赋值的*/
		frame6_info.F6UpgradeSubStatus = 50;
		send_frame(UPDATE_UPGRADE_VALI_FRAME, (void *)&frame6_info, sizeof(update_frame6_info));
		return 0;

	}

	memcpy(&frame_info.FrameLength, data_buf + FRAME_LENGTH_OFFSET, sizeof(u16));

	ret = receive_data_check(data_buf, index);
	if(ret){
		printk("START FRAME is check failed\n");
		return ret;
	}

	memcpy(pack_info_data, data_buf + FRAME_DATA_OFFSET , PACK_INFO_LENGTH);
	pack_info = *(update_pack_info *)pack_info_data;

	ak_update.frame_size = pack_info.PFrameSize + DATA_FRAME_HEDA_LEN + sizeof(frame_info.CS);

	ak_update.subpack_size = ak_update.frame_size * pack_info.FrameQuantityAtOnce;

	if(pack_info.FrameTotalCount % pack_info.FrameQuantityAtOnce){
		ak_update.last_subpack_size = ak_update.frame_size * (pack_info.FrameTotalCount % pack_info.FrameQuantityAtOnce);
		ak_update.subpack_num = pack_info.FrameTotalCount / pack_info.FrameQuantityAtOnce + 1;
	} else {
		ak_update.subpack_num = pack_info.FrameTotalCount / pack_info.FrameTotalCount;
		ak_update.last_subpack_size = ak_update.subpack_size;
	}
	printk("EnableOTA=%d, Timeouttime=%d, PFrameSize=%d, PkgSize=%d, FrameTotalCount=%d, Pkgversion=%d, FrameQuantityAtOnce=%d, frame_size=%d, subpack_size=%d, subpack_num=%d, last_subpack_size =%d\n",pack_info.EnableOTA, pack_info.TimeoutTime, pack_info.PFrameSize, pack_info.PkgSize, pack_info.FrameTotalCount, pack_info.PkgVersion, pack_info.FrameQuantityAtOnce,ak_update.frame_size,ak_update.subpack_size, ak_update.subpack_num, ak_update.last_subpack_size);

	if(ak_update.firmware_data_buf != NULL) {
		kfree(ak_update.firmware_data_buf);
		printk("firmware data buf not null\n");
	}

	memset(&frame_info, 0, sizeof(update_frame_info));

	complete(&ak_update.malloc_block);

	send_frame(UPDATE_INTERVAL_FRAME, (void *)&frame_info.FrameOfStar, sizeof(frame_info.FrameOfStar));
	return 0;
};

static int update_guard_process_thread(void *data)
{
	u32 datasum_last = 0;
	int error_num = 0;

	while(1) {

		if(frame_info.DataFrameIndex == pack_info.FrameTotalCount - 1){
			printk("Data transfer complete\n");
			break;
		}

		if(datasum_last == datasum){
			printk("the host does not send data for a long time\n");
			subpack_copy = 0;
			frame_info.BatchIndex = 0;
			/*datasum_last = frame_info.FrameOfStar;*/
			error_num++;
			send_frame(UPDATE_INTERVAL_FRAME, (void *)&frame_info.FrameOfStar, sizeof(frame_info.FrameOfStar));
			msleep(500);

		} else {
			datasum_last = datasum;
		}

		if(error_num >= 5){
			VerifyStatus = 0;
			send_frame(UPDATE_FINISH_FRAME, (void *)&VerifyStatus, sizeof(VerifyStatus));
			printk("Disconnect more than three times and re-upgrade\n");
			msleep(500);
			norflash_oprate(1, 0);
		}
		msleep(2000);

	}
	return 0;
}

static int update_frame_process_thread(void *data)
{
	static int data_frame_err;
	int ret = -1, copy_len = 0;
	int norflash_flag = 0, frame_data_copy_len = 0;

	while(1){
		wait_for_completion(&ak_update.data_block);
		if (ak_update.firmware_data_buf == NULL)
			printk("firmware_data_buf=NULL\n");

		memset(ak_update.frame_data_buf, 0, ak_update.frame_size);
		memcpy(ak_update.frame_data_buf, ak_update.subpack_data_buf + frame_data_copy_len, ak_update.frame_size);
		frame_data_copy_len += ak_update.frame_size;

		memcpy(&frame_info.FrameLength, ak_update.frame_data_buf + FRAME_LENGTH_OFFSET, sizeof(u16));
		memcpy((void *)&frame_info.DataFrameIndex, ak_update.frame_data_buf + FRAME_DATA_OFFSET, sizeof(u32));
		frame_info.CS = ak_update.firmware_data_buf[frame_info.FrameLength - 1];
		frame_info.FrameNumber = ak_update.frame_data_buf[FRAME_NUMBER_OFFSET];

		ret = receive_data_check(ak_update.frame_data_buf, UPDATE_DATA_FRAME);
		if((ret < 0) || (frame_info.DataFrameIndex != frame_info.CurrentIndex + frame_info.BatchIndex)){
			if(data_frame_err > 3){
				printk("this data more than three error frameindex=%d\n",frame_info.DataFrameIndex);
				data_frame_err = 0;
				VerifyStatus = 0;
				send_frame(UPDATE_FINISH_FRAME, (void *)&VerifyStatus, sizeof(VerifyStatus));
				complete(&ak_update.reset_block);
			} else {
				printk("frame data error frameindex=%d, Currentindex=%d, Batchindex=%d\n",frame_info.DataFrameIndex, frame_info.CurrentIndex, frame_info.BatchIndex);
				data_frame_err++;
				frame_info.FrameOfStar = frame_info.CurrentIndex;
				frame_info.BatchIndex = 0;
				send_frame(UPDATE_INTERVAL_FRAME, (void *)&frame_info.FrameOfStar, sizeof(frame_info.FrameOfStar));
			}
			continue;
		}

		data_frame_err = 0;
		copy_len = frame_info.FrameLength - DATA_FRAME_HEDA_LEN - sizeof(frame_info.CS);
		memcpy(ak_update.firmware_data_buf + frame_info.Memcpy_count, ak_update.frame_data_buf + DATA_FRAME_HEDA_LEN,  copy_len);
		frame_info.Memcpy_count += copy_len;

		if(frame_info.Memcpy_count > pack_info.PkgSize)
		{
			printk("send data length %d is greater than Pkgsize=%d, frame_info.DataFrameIndex=%d, frame_info.FrameLength=%d\n", frame_info.Memcpy_count, pack_info.PkgSize, frame_info.DataFrameIndex, frame_info.FrameLength);
		};

		if(frame_info.DataFrameIndex == pack_info.FrameTotalCount - 1){
			ret = get_firmware_head_info(ak_update.firmware_data_buf);
			if(ret < 0) {
				VerifyStatus = 0;
				norflash_flag = 1;
				printk("upgrade data md5 check error\n");
			} else {
				VerifyStatus = 1;
				printk("upgrade data receive complete\n");
			}
			send_frame(UPDATE_FINISH_FRAME, (void *)&VerifyStatus, sizeof(VerifyStatus));
			norflash_oprate(norflash_flag, 0);
		} else {
			if(frame_info.BatchIndex == pack_info.FrameQuantityAtOnce - 1){
				frame_info.BatchIndex = 0;
				frame_info.CurrentIndex += pack_info.FrameQuantityAtOnce;
				frame_info.FrameOfStar = frame_info.CurrentIndex;
				send_frame(UPDATE_INTERVAL_FRAME, (void *)&frame_info.FrameOfStar, sizeof(frame_info.FrameOfStar));
				printk("####Currentindex=%d######\n", frame_info.CurrentIndex);
				frame_data_copy_len = 0;
				continue;
			} else{
				complete(&ak_update.data_block);
			}

			frame_info.BatchIndex += 1;
		}
	}

	return 0;
}

static int ak_update_thread(void *data)
{

	wait_for_completion(&ak_update.malloc_block);
	if(ak_update.firmware_data_buf == NULL){
		ak_update.firmware_data_buf = kmalloc(pack_info.PkgSize, GFP_KERNEL);
		if(ak_update.firmware_data_buf == NULL){
			printk("kmalloc cdc data buf is failed\n");
		}
	}

	if(protocol_mode == UPDATE_MODE){
		if(ak_update.subpack_data_buf == NULL){
			ak_update.subpack_data_buf = kmalloc(ak_update.subpack_size, GFP_KERNEL);
			if(ak_update.subpack_data_buf == NULL){
				printk("kmalloc put together buf is failed\n");
			}
		}

		if(ak_update.frame_data_buf == NULL){
			ak_update.frame_data_buf = kmalloc(ak_update.frame_size, GFP_KERNEL);
			if(ak_update.frame_data_buf == NULL){
				printk("kmalloc put together buf is failed\n");
			}
		}
	}
	return 0;
}

static int ak_reset_thread(void *data)
{
	int norflash_flag = 1;
	wait_for_completion(&ak_update.reset_block);
	norflash_oprate(norflash_flag, 0);
	return 0;
}

static int update_frame_type_judge(void *buf)
{
	int i = 0;
	u8 update_frame1_head[HEAD_LENGTH + 1] = {0x08, 0xEE, 0x00, 0x00, 0x00, 0x07,0x01};
	u8 data_head[HEAD_LENGTH + 1];

	memcpy(data_head, buf, HEAD_LENGTH + 1);
	for(i = 0; i < HEAD_LENGTH + 1 ; i++)
	{
		if(data_head[i] != update_frame1_head[i])
			return UPDATE_DATA_FRAME;
	}
	return UPDATE_START_FRAME;
};

int get_update_data(void *buf, int len)
{
	int ret = -1;
	int  frame_index;
	static int subpack_count, subpack_size;
	u8	*data_buf = NULL;
	data_buf = (u8 *)buf;
	switch(protocol_mode){
		case LOG_MODE:
			frame_info.FrameNumber = data_buf[FRAME_NUMBER_OFFSET];
			memcpy(&frame_info.FrameLength, data_buf + FRAME_LENGTH_OFFSET, sizeof(u16));

			ret = receive_data_check(data_buf, frame_info.FrameNumber);
			if(ret){
				printk("frame check error in log mode\n");
				goto error;
			}

			if(frame_info.FrameNumber == LOG_START_FRAME)
				wake_up_process(log_process_thread);
			else if(frame_info.FrameNumber == LOG_FINISH_FRAME) {
				printk("log transfer complete\n");
				complete(&ak_update.reset_block);
			}

			break;

		case UPDATE_MODE:
			frame_index = update_frame_type_judge(buf);
			switch(frame_index) {
				case UPDATE_START_FRAME:
					ret =  update_start_frame_process(buf, frame_index);
					if(ret)
						goto error;
					subpack_copy = 0;
					subpack_count = 1;
					wake_up_process(guard_thread);
					break;
				case UPDATE_DATA_FRAME:
					if(subpack_count == ak_update.subpack_num)
						subpack_size = ak_update.last_subpack_size;
					else
						subpack_size = ak_update.subpack_size;
					datasum++;

					memcpy(ak_update.subpack_data_buf + subpack_copy, buf, len);
					subpack_copy += len;

					if(subpack_copy == subpack_size){
						printk("subpack_copy=%d\n", subpack_copy);
						complete(&ak_update.data_block);
						subpack_copy = 0;
						subpack_count++;
					}
					break;
				default:
					printk("frame number error(%d) in update mode\n", frame_index);
					goto error;
			}
			break;
		default:
			printk("CDC mode(%d) is error\n ", protocol_mode);
			goto error;
	}
	return 0;
error:
	complete(&ak_update.reset_block);
	return -1;
};

static int usb_driver_enhance(void)
{
	        /* PB25: PWR_HOLD */
	*(volatile unsigned int *)(0xb0060000 + 0x40) = (1 << 5); //PCINTC 0
	*(volatile unsigned int *)(0xb0060000 + 0x120) = (1 << 5); //PCMSKS 1
	*(volatile unsigned int *)(0xb0060000 + 0x124) = (1 << 2); //PCPAT1C 0
	printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	msleep(100);

	return 0;
}


int cdc_serial_update(void)
{
	struct task_struct *update_thread;
	struct task_struct *reset_thread;
	struct task_struct *update_data_process_thread;
	int flash_flag = 1, mode_flag = 0;

	direct_read_norflash(&mode_flag, NORFLASH_OFFSET, sizeof(int));
	printk("######mode_flag=0x%x##########\n",mode_flag);
	usb_driver_enhance();

	update_thread = kthread_create(ak_update_thread, NULL, "usb-serial-update");
	if(IS_ERR(update_thread))
	{
		return PTR_ERR(update_thread);
	}

	reset_thread = kthread_create(ak_reset_thread, NULL, "reset-update");
	if(IS_ERR(reset_thread))
	{
		return PTR_ERR(reset_thread);
	}

	if(mode_flag == LOG_SIGNATURE){
		log_process_thread = kthread_create(log_frame_process_thread, NULL, "log_frame-update");
		if(IS_ERR(log_process_thread))
		{
			return PTR_ERR(log_process_thread);
		}
		protocol_mode = LOG_MODE;
		norflash_oprate(flash_flag, 1);
	} else {
		flash_flag = UPDATE_SIGNATURE;
		direct_erase_norflash(NORFLASH_OFFSET, sizeof(int));
		direct_write_norflash(&flash_flag, NORFLASH_OFFSET, sizeof(int));

		update_data_process_thread = kthread_create(update_frame_process_thread, NULL, "data_frame-update");
		if(IS_ERR(update_data_process_thread))
		{
			return PTR_ERR(update_data_process_thread);
		}
		protocol_mode = UPDATE_MODE;
		init_completion(&ak_update.data_block);
		wake_up_process(update_data_process_thread);

		guard_thread = kthread_create(update_guard_process_thread, NULL, "data_frame-update");
		if(IS_ERR(guard_thread))
		{
			return PTR_ERR(guard_thread);
		}
	}


	init_completion(&ak_update.malloc_block);
	init_completion(&ak_update.reset_block);
	wake_up_process(update_thread);
	wake_up_process(reset_thread);

	return 0;
}



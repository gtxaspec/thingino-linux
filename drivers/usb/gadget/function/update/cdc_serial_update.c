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
#include "cdc_serial_update.h"


static u32 datasum = 1;            //守护进程的数据检查
static u32 subpack_copy_lenth;     //子包拷贝数据长度
static u8 verify_status;           //接收成功与否的状态
static u32 error_count = 0;        //传输错误计数
static u32 subpack_total_count=0;  //子包数量
static u8 cdc_send_data_buf[512];  //发送的数据包
static u16 send_data_length = 0;   //发送的数据长度

jz_update_frame_info_t jz_frame_info;        //接收Data Frame的相关信息
jz_frame1_update_pack_info_t jz_pack_info;   //接收Frame1的相关信息
jz_update_firmware_info_t jz_update;         //接收固件的相关信息
struct task_struct *daemon_thread_head;

static int jz_receive_data_check(u8 *buf, u8 index)
{
	int i;
	u8 receive_data_head[JZ_HEAD_LENGTH] = {0x80, 0x00, 0xEE};
	u8 data_head[JZ_HEAD_LENGTH];
	u8 *data_buf;
	u8 crc = 0;

	data_buf = buf;

	jz_frame_info.cs = data_buf[jz_frame_info.frame_length - 1];

	for(i = 0; i < jz_frame_info.frame_length - 1; i++) {
		crc += data_buf[i];
	}

	if(crc != jz_frame_info.cs) {
		printk("#########receive frame=%d cs is error(%x),%x########\n", index, crc, jz_frame_info.cs);
		return -1;
	}

	memcpy(data_head, buf, JZ_HEAD_LENGTH);

	for(i = 0; i < JZ_HEAD_LENGTH ; i++) {//数据包头部校验
		if(data_head[i] != receive_data_head[i]) {
			printk("receive frame=%d, data[%d] head is error(%d)\n", index, i, data_head[i]);
			return -1;
		}
	}

	return 0;
}

static int jz_frame_packet(u8 index, void *data, int data_len)
{
	u8 crc = 0;
	int i = 0;
	u8 send_data_head[JZ_HEAD_LENGTH] = {0x81,0x00,0xEE};

	send_data_length = 0;                                                                                            //为了清除之前的数据
	memset(cdc_send_data_buf, 0, sizeof(cdc_send_data_buf));                                                         //为了清除之前的数据

	memcpy((void *)cdc_send_data_buf, send_data_head, JZ_HEAD_LENGTH);                                               //拷贝头部信息（3个字节）
	cdc_send_data_buf[JZ_FRAME_NUMBER_OFFSET] = index;                                                               //拷贝第4个字节
	send_data_length = JZ_HEAD_LENGTH + sizeof(index) + sizeof(send_data_length) + data_len + sizeof(crc);           //计算数据包长度信息
	memcpy((void *)cdc_send_data_buf + JZ_FRAME_LENGTH_OFFSET,  (void *)&send_data_length, sizeof(send_data_length));//拷贝长度信息

	if(data != NULL) {
		memcpy(cdc_send_data_buf + JZ_FRAME_DATA_OFFSET, data, data_len);                                            //拷贝数据信息
	}

	cdc_send_data_buf[send_data_length - 1] = 0;                                                                     //将最后一位CS清除
	for(i = 0; i < send_data_length -1; i++) {                                                                       //计算最后一位CS
		crc += cdc_send_data_buf[i];
	}
	cdc_send_data_buf[send_data_length - 1] = crc;

	return 0;
}

static int jz_send_frame(void *buf, int buf_len)
{
	int count;
	count = gs_write_cdc(buf, buf_len);//串口发送
	if(count != buf_len) {
		printk("send frame data is failed,count=%d,buf_len=%d\n",count,buf_len);
		return -1;
	}
	return 0;
}

static int jz_kmalloc_init(void)
{
	if(jz_update.firmware_data_buf == NULL) {
		jz_update.firmware_data_buf = kmalloc(jz_pack_info.firmware_size, GFP_KERNEL);
		if(jz_update.firmware_data_buf == NULL) {
			printk("kmalloc cdc data buf is failed\n");
		}
	}

	if(jz_update.batch_pack_data_buf == NULL) {
		jz_update.batch_pack_data_buf = kmalloc(jz_update.batch_pack_size, GFP_KERNEL);
		if(jz_update.batch_pack_data_buf == NULL) {
			printk("kmalloc put together buf is failed\n");
		}
	}

	if(jz_update.frame_data_buf == NULL) {
		jz_update.frame_data_buf = kmalloc(jz_update.frame3_size, GFP_KERNEL);
		if(jz_update.frame_data_buf == NULL) {
			printk("kmalloc put together buf is failed\n");
		}
	}

	return 0;
}

static void jz_norflash_oprate(int flag)
{
	int flash_flag = 0;

	switch (flag)
	{
		case FLASH_BURN_FIRMWARE_FLAG:
			direct_erase_norflash(jz_update.img_offset, jz_update.img_size);
			direct_write_norflash(jz_update.firmware_data_buf + HEAD_INFO_RESERVED, jz_update.img_offset, jz_update.img_size);
			flash_flag = NORFALSH_RESET_SIGNATURE;
			direct_erase_norflash(NORFLASH_OFFSET, sizeof(int));
			direct_write_norflash(&flash_flag, NORFLASH_OFFSET, sizeof(int));
			machine_restart("Restart the system after the upgrade is complete");
			break;

		case FLASH_UPDATE_RESET_FLAG:
			flash_flag = NORFALSH_RESET_SIGNATURE;
			direct_erase_norflash(NORFLASH_OFFSET, sizeof(int));
			direct_write_norflash(&flash_flag, NORFLASH_OFFSET, sizeof(int));
			machine_restart("Restart the system after the upgrade is complete");
			break;

		case FLASH_UPDATE_SET_FLAG:
			flash_flag = UPDATE_SIGNATURE;
			direct_erase_norflash(NORFLASH_OFFSET, sizeof(int));
			direct_write_norflash(&flash_flag, NORFLASH_OFFSET, sizeof(int));
			break;

		default:
			break;
	}
}

static int jz_get_firmware_head_info(void *buf)
{
	int i;
	jz_firmware_head_t head;
	unsigned char buf_md5sum[16];

	memcpy((void *)&head, buf, sizeof(jz_firmware_head_t));
	jz_update.img_offset = head.img_offset;
	jz_update.img_size = head.img_size;
	buffer_md5sum_0((char *)jz_update.firmware_data_buf + HEAD_INFO_RESERVED, buf_md5sum, jz_update.img_size);

	for(i = 0; i < MD5SUM_LENGTH; i++) {
		if(buf_md5sum[i] != head.md5[i]) {
			printk("firmware md5 check is error,buf_md5sum[15]=%d, head.md5[15]=%d\n", buf_md5sum[15], head.md5[16]);
			return -1;
		}
	}

	return 0;
}

static int jz_update_start_frame_process(void *buf, int index)
{
	u8	test[50]={0};
	u32	*send_buf_pre = NULL;
	int	ret = -1;
	u8	pack_info_data[JZ_FRAME1_PACK_INFO_LENGTH];
	u8	*data_buf = NULL;

	data_buf = (u8 *)buf;
	memcpy(test, data_buf, 40);

	memcpy(&jz_frame_info.frame_length, data_buf + JZ_FRAME_LENGTH_OFFSET, sizeof(u16));                  //将frame1中的数据长度提取出来放入全局变量 jz_frame_info.frame_length 中

	ret = jz_receive_data_check(data_buf, index);
	if(ret) {
		printk("SOF FRAME is check failed\n");
		return ret;
	}

	memcpy(pack_info_data, data_buf + JZ_FRAME_DATA_OFFSET , JZ_FRAME1_PACK_INFO_LENGTH);
	jz_pack_info = *(jz_frame1_update_pack_info_t *)pack_info_data;                                       //将接受的buf内容(除去头部信息与数据长度)放入全局变量 jz_pack_info 中
	jz_update.frame3_size = jz_pack_info.subpack_size + JZ_DATA_FRAME_HEDA_LEN + sizeof(jz_frame_info.cs);//这是计算Frame3大小 512
	jz_update.batch_pack_size = jz_update.frame3_size * jz_pack_info.subpackcnt_atonce;                   //一次性发包数量*Frame3大小

	if(jz_pack_info.firmware_size % jz_pack_info.subpack_size) {
		subpack_total_count = jz_pack_info.firmware_size / jz_pack_info.subpack_size + 1;
	} else {
		subpack_total_count = jz_pack_info.firmware_size / jz_pack_info.subpack_size;
	}

	if(subpack_total_count % jz_pack_info.subpackcnt_atonce) {
		jz_update.last_batch_pack_size = jz_update.frame3_size * (subpack_total_count % jz_pack_info.subpackcnt_atonce);//Frame3大小 *（升级文件子包数量 % 一次性发包数量）--最后一个子包组的数据大小
		jz_update.batch_pack_cnt = subpack_total_count / jz_pack_info.subpackcnt_atonce + 1;                            //（升级文件子包数量 / 一次性发包数量）+1 总共发送子包组的次数
	} else {
		jz_update.batch_pack_cnt = subpack_total_count / jz_pack_info.subpackcnt_atonce;
		jz_update.last_batch_pack_size = jz_update.batch_pack_size;
	}

	printk("subpack_size=%d, firmware_size=%d, subpack_total_count=%d, subpackcnt_atonce=%d, firmware_version=%d, frame3_size=%d, batch_pack_size=%d, batch_pack_cnt=%d, last_batch_pack_size =%d\n", jz_pack_info.subpack_size, jz_pack_info.firmware_size, subpack_total_count, jz_pack_info.subpackcnt_atonce, jz_pack_info.firmware_version, jz_update.frame3_size,jz_update.batch_pack_size, jz_update.batch_pack_cnt, jz_update.last_batch_pack_size);

	if(jz_update.firmware_data_buf != NULL) {
		kfree(jz_update.firmware_data_buf);
		kfree(jz_update.batch_pack_data_buf);
		kfree(jz_update.frame_data_buf);
		jz_update.firmware_data_buf=NULL;
		jz_update.batch_pack_data_buf=NULL;
		jz_update.frame_data_buf=NULL;
		printk("firmware data buf not null\n");
	}

	memset(&jz_frame_info, 0, sizeof(jz_update_frame_info_t));

	jz_kmalloc_init();

	send_buf_pre=kmalloc(sizeof(jz_frame_info.frame_num_of_start_send), GFP_KERNEL);
	memcpy(send_buf_pre,(void *)&jz_frame_info.frame_num_of_start_send, sizeof(jz_frame_info.frame_num_of_start_send));
	jz_frame_packet(JZ_UPDATE_ACK_FRAME, (void *)send_buf_pre, sizeof(jz_frame_info.frame_num_of_start_send));
	jz_send_frame(cdc_send_data_buf,send_data_length);//发送Frame2
	kfree(send_buf_pre);

	return 0;
}

static int jz_update_daemon_process_thread(void *data)
{
	u32 datasum_last = 0;
	u32	*send_buf_pre = NULL;
	while(1) {

		if(jz_frame_info.data_frame_index == subpack_total_count - 1) {
			printk("Data transfer complete\n");
			break;
		}

		if(datasum_last == datasum) {
			printk("the host does not send data for a long time\n");
			subpack_copy_lenth = 0;
			jz_frame_info.batch_index = 0;
			error_count++;

			send_buf_pre=kmalloc(sizeof(jz_frame_info.frame_num_of_start_send), GFP_KERNEL);
			memcpy(send_buf_pre,(void *)&jz_frame_info.frame_num_of_start_send, sizeof(jz_frame_info.frame_num_of_start_send));
			jz_frame_packet(JZ_UPDATE_ACK_FRAME, (void *)send_buf_pre, sizeof(jz_frame_info.frame_num_of_start_send));//发送Frame2
			jz_send_frame(cdc_send_data_buf,send_data_length);
			kfree(send_buf_pre);

			msleep(500);
		} else {
			datasum_last = datasum;
			error_count = 0;
		}

		if(error_count >= 5) {
			verify_status = 0;

			send_buf_pre=kmalloc(sizeof(verify_status), GFP_KERNEL);
			memcpy(send_buf_pre,(void *)&verify_status, sizeof(verify_status));
			jz_frame_packet(JZ_UPDATE_EOF_FRAME, (void *)send_buf_pre, sizeof(verify_status));//发送Frame5
			jz_send_frame(cdc_send_data_buf,send_data_length);
			kfree(send_buf_pre);

			printk("Disconnect more than three times and re-upgrade\n");
			msleep(500);
			jz_norflash_oprate(FLASH_UPDATE_RESET_FLAG);
		}
		msleep(2000);
	}

	return 0;
}

static int jz_update_frame_process_thread(void *data)
{
	static int data_frame_err;
	int ret = -1, copy_len = 0;
	int norflash_flag = FLASH_BURN_FIRMWARE_FLAG, frame_data_copy_len = 0;
	u32	*send_buf_pre = NULL;

	while(1) {
		wait_for_completion(&jz_update.data_block);//阻塞  线程进入休眠
		if (jz_update.firmware_data_buf == NULL) {
			printk("firmware_data_buf=NULL\n");
		}

		memset(jz_update.frame_data_buf, 0, jz_update.frame3_size);
		memcpy(jz_update.frame_data_buf, jz_update.batch_pack_data_buf + frame_data_copy_len, jz_update.frame3_size);  //jz_update.frame_data_buf
		frame_data_copy_len += jz_update.frame3_size;

		memcpy(&jz_frame_info.frame_length, jz_update.frame_data_buf + JZ_FRAME_LENGTH_OFFSET, sizeof(u16));           //一个Frame3的数据长度
		memcpy((void *)&jz_frame_info.data_frame_index, jz_update.frame_data_buf + JZ_FRAME_DATA_OFFSET, sizeof(u32)); //Frame3子包序列号
		jz_frame_info.cs = jz_update.firmware_data_buf[jz_frame_info.frame_length - 1];                                //Frame3校验位
		jz_frame_info.frame_number = jz_update.frame_data_buf[JZ_FRAME_NUMBER_OFFSET];                                 //什么Frame进行保存

		ret = jz_receive_data_check(jz_update.frame_data_buf, JZ_UPDATE_DATA_FRAME);
		if((ret < 0) || (jz_frame_info.data_frame_index != jz_frame_info.current_index + jz_frame_info.batch_index)) {

			if(data_frame_err > 3) {
				printk("this data more than three error frameindex=%d\n",jz_frame_info.data_frame_index);
				data_frame_err = 0;
				verify_status = 0;

				send_buf_pre=kmalloc(sizeof(verify_status), GFP_KERNEL);
				memcpy(send_buf_pre,(void *)&verify_status, sizeof(verify_status));
				jz_frame_packet(JZ_UPDATE_EOF_FRAME, (void *)send_buf_pre, sizeof(verify_status));
				jz_send_frame(cdc_send_data_buf,send_data_length);
				kfree(send_buf_pre);

				complete(&jz_update.reset_block);
			} else {
				printk("frame data error frameindex=%d, Currentindex=%d, Batchindex=%d\n",jz_frame_info.data_frame_index, jz_frame_info.current_index, jz_frame_info.batch_index);
				data_frame_err++;
				jz_frame_info.frame_num_of_start_send = jz_frame_info.current_index;
				jz_frame_info.batch_index = 0;

				send_buf_pre=kmalloc(sizeof(jz_frame_info.frame_num_of_start_send), GFP_KERNEL);
				memcpy(send_buf_pre,(void *)&jz_frame_info.frame_num_of_start_send, sizeof(jz_frame_info.frame_num_of_start_send));
				jz_frame_packet(JZ_UPDATE_ACK_FRAME, (void *)send_buf_pre, sizeof(jz_frame_info.frame_num_of_start_send));
				jz_send_frame(cdc_send_data_buf,send_data_length);
				kfree(send_buf_pre);

			}

			continue;
		}

		data_frame_err = 0;
		copy_len = jz_frame_info.frame_length - JZ_DATA_FRAME_HEDA_LEN - sizeof(jz_frame_info.cs);
		memcpy(jz_update.firmware_data_buf + jz_frame_info.memcpy_count, jz_update.frame_data_buf + JZ_DATA_FRAME_HEDA_LEN,  copy_len);//开始拷贝固件信息到 firmware_data_buf
		jz_frame_info.memcpy_count += copy_len;

		if(jz_frame_info.data_frame_index == subpack_total_count - 1) {//判断是否发送完成

			ret = jz_get_firmware_head_info(jz_update.firmware_data_buf);
			if(ret < 0) {
				verify_status = 0;
				norflash_flag = FLASH_UPDATE_RESET_FLAG;
				printk("upgrade data md5 check error\n");
			} else {
				verify_status = 1;
				printk("upgrade data receive complete\n");
			}

			send_buf_pre=kmalloc(sizeof(verify_status), GFP_KERNEL);
			memcpy(send_buf_pre,(void *)&verify_status, sizeof(verify_status));
			jz_frame_packet(JZ_UPDATE_EOF_FRAME, (void *)send_buf_pre, sizeof(verify_status));
			jz_send_frame(cdc_send_data_buf,send_data_length);
			kfree(send_buf_pre);

			jz_norflash_oprate(norflash_flag);
		} else {
			if(jz_frame_info.batch_index == jz_pack_info.subpackcnt_atonce - 1) {

				jz_frame_info.batch_index = 0;
				jz_frame_info.current_index += jz_pack_info.subpackcnt_atonce;
				jz_frame_info.frame_num_of_start_send = jz_frame_info.current_index;

				send_buf_pre=kmalloc(sizeof(jz_frame_info.frame_num_of_start_send), GFP_KERNEL);
				memcpy(send_buf_pre,(void *)&jz_frame_info.frame_num_of_start_send, sizeof(jz_frame_info.frame_num_of_start_send));
				jz_frame_packet(JZ_UPDATE_ACK_FRAME, (void *)send_buf_pre, sizeof(jz_frame_info.frame_num_of_start_send));
				jz_send_frame(cdc_send_data_buf,send_data_length);
				kfree(send_buf_pre);

				printk("####Currentindex=%d######\n", jz_frame_info.current_index);
				frame_data_copy_len = 0;
				continue;
			} else {
				complete(&jz_update.data_block);//自我唤醒不阻塞了 直接处理下一个子包 jz_update_frame_process_thread 线程
			}

			jz_frame_info.batch_index += 1;
		}
	}

	return 0;
}

static int jz_reset_thread(void *data)
{
	wait_for_completion(&jz_update.reset_block);
	jz_norflash_oprate(FLASH_UPDATE_RESET_FLAG);
	return 0;
}

static int update_frame_type_judge(void *buf , int len )
{
	int i=0;
	u8 data_head[1000];
	u8 jz_data_head_raw[JZ_HEAD_LENGTH]={0x80,0x00,0xEE};
	u8 jz_data_head[JZ_HEAD_LENGTH + 1];
	u8 data_judge_flag = 0;

	memcpy(data_head, buf, len);

	for(i = 0; i < JZ_HEAD_LENGTH ; i++) {
		if(data_head[i]==jz_data_head_raw[i]) {
			if(i == JZ_HEAD_LENGTH-1) {
				data_judge_flag = JZ_JUDGE;
			}
		} else {
			printk("This data is not JZ data!!! \n");
			break ;
		}
	}

	if (data_judge_flag == JZ_JUDGE) {
		memcpy(jz_data_head, buf, JZ_HEAD_LENGTH + 1);

		switch (jz_data_head[JZ_HEAD_FRAME_CHECK_NODE]) {

			case JZ_UPDATE_SOF_FRAME:
				return JZ_UPDATE_SOF_FRAME;
				break;

			case JZ_UPDATE_ACK_FRAME:
				return JZ_UPDATE_ACK_FRAME;
				break;

			case JZ_UPDATE_DATA_FRAME:
				return JZ_UPDATE_DATA_FRAME;
				break;

			default:
				printk("JZ frame type judge error!!!!!!!!!\n");
				break;
		}

	} else if (data_judge_flag == 0) {
		return CDC_CONNET_INDEX;
	}

}

int get_update_data(void *buf, int len)
{
	int ret = -1;
	int frame_index;
	static int batch_pack_count, batch_pack_size_tmp;

	frame_index = update_frame_type_judge(buf , len);

	switch(frame_index) {

		case JZ_UPDATE_SOF_FRAME:
				ret =  jz_update_start_frame_process(buf, frame_index);
				if(ret)
					goto error;

				error_count=0;
				subpack_copy_lenth = 0;
				batch_pack_count = 1;//发送子包组的次数
				wake_up_process(daemon_thread_head);
			break;

		case JZ_UPDATE_DATA_FRAME:
				//判断发送子包组的次数是否是最后一次
				if(batch_pack_count == jz_update.batch_pack_cnt)
					batch_pack_size_tmp = jz_update.last_batch_pack_size;
				else
					//一组Frame3大小
					batch_pack_size_tmp = jz_update.batch_pack_size;

				datasum++;
				memcpy(jz_update.batch_pack_data_buf + subpack_copy_lenth, buf, len);
				subpack_copy_lenth += len;
				//是否完整的接收了一组包的大小的数据
				if(subpack_copy_lenth == batch_pack_size_tmp) {
					printk("subpack_copy_lenth=%d\n", subpack_copy_lenth);
					//唤醒 jz_update_frame_process_thread 线程
					complete(&jz_update.data_block);
					subpack_copy_lenth = 0;
					batch_pack_count++;
				}
			break;

		case CDC_CONNET_INDEX:
			printk("CDC CONNET!!!!\n");
		break;

		}
	return 0;

error:
	complete(&jz_update.reset_block);
	return -1;
}

static int usb_driver_enhance(void)
{
	/* PB25: PWR_HOLD */
	*(volatile unsigned int *)(0xb0060000 + 0x40) = (1 << 5);  //PCINTC 0
	*(volatile unsigned int *)(0xb0060000 + 0x120) = (1 << 5); //PCMSKS 1
	*(volatile unsigned int *)(0xb0060000 + 0x124) = (1 << 2); //PCPAT1C 0
	printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	msleep(100);

	return 0;
}

int cdc_serial_update(void)
{
	struct task_struct *reset_thread_head;
	struct task_struct *update_data_process_thread_head;
	int flash_flag = 1, mode_flag = 0;

	direct_read_norflash(&mode_flag, NORFLASH_OFFSET, sizeof(int));
	printk("######mode_flag=0x%x##########\n",mode_flag);
	usb_driver_enhance();

	reset_thread_head = kthread_create(jz_reset_thread, NULL, "reset-thread");
	if(IS_ERR(reset_thread_head)) {
		return PTR_ERR(reset_thread_head);
	}

	jz_norflash_oprate(FLASH_UPDATE_SET_FLAG);

	update_data_process_thread_head = kthread_create(jz_update_frame_process_thread, NULL, "data_frame-thread");
	if(IS_ERR(update_data_process_thread_head)) {
		return PTR_ERR(update_data_process_thread_head);
	}

	init_completion(&jz_update.data_block);
	wake_up_process(update_data_process_thread_head);

	daemon_thread_head = kthread_create(jz_update_daemon_process_thread, NULL, "daemon-thread");
	if(IS_ERR(daemon_thread_head)) {
		return PTR_ERR(daemon_thread_head);
	}

	init_completion(&jz_update.reset_block);
	wake_up_process(reset_thread_head);

	return 0;
}

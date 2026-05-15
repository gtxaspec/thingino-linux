#include "include/user/osd.h"
//#include "include/user/dates.h"
#include "include/osd/imp_common.h"
#include "include/osd/imp_isp.h"
#include "include/osd/imp_osd.h"
#include "include/osd/isp_osd.h"
#include "include/osd/osd_mem.h"
#include "include/tx-isp-common.h"
//#include "include/osd/logodata_100x100_bgra.h"
#include "include/osd/bgramapinfo.h"
#include "include/user/osd.h"
#include <linux/delay.h>

extern IMP_Sample_OsdParam ipuosdparam;
extern IMP_Sample_OsdParam isposdparam;

int Days[12] = {31,29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
// 判断是否为闰年的函数
int isLeapYear(int year) {
	if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
		return 1;
	} else {
		return 0;
	}
}

void calculate_dates(Date *current_date, Date *last_date){
	if(last_date->second > 59)
	{
		last_date->second = 0;
		last_date->minute++;
		if(last_date->minute > 59)
		{
			last_date->minute = 0;
			last_date->hour++;
			if(last_date->hour > 23)
			{
				last_date->hour = 0;
				last_date->day++;
				if (isLeapYear(last_date->year)) {
					Days[2-1] = 29;
				} else {
					Days[2-1] = 28;
				}
				if(last_date->day >= Days[last_date->month-1])
				{
					last_date->day = 1;
					last_date->month++;
					if(last_date->month > 12)
					{
						last_date->month = 1;
						last_date->year++;
					}
				}
			}
		}
	}
	current_date->year = last_date->year;
	current_date->month = last_date->month;
	current_date->day = last_date->day;
	current_date->hour = last_date->hour;
	current_date->minute = last_date->minute;
	current_date->second = last_date->second;
}


int update_time(void *p)
{
	IMP_Sample_OsdParam *isposdparam = (IMP_Sample_OsdParam*)p;
	/* generate time */
	int ret = 0;
	int i = 0, j = 0;
	char DateStr[40] = { 0 };
	void *dateData = NULL;
	uint32_t *data = (void *)isposdparam->ptimestamps;
	IMPIspOsdAttrAsm stISPOSDAsm;
	Date current_date;
	Date last_date = {.year = 1970, .month = 1, .day = 1, .hour = 0, .minute = 0, .second = 0};
	memset(&current_date, 0, sizeof(Date));

	printk("isposd Thread started\n");
	while (!kthread_should_stop()) {
		unsigned int len = 0;
		int fontadv = 0;
		int penpos_t = 0;
		int penpos = 0;

		calculate_dates(&current_date, &last_date);
		sprintf(DateStr, "%d-%02d-%02d %02d:%02d:%02d", current_date.year, current_date.month, current_date.day, current_date.hour, current_date.minute, current_date.second);
		len = strlen(DateStr);  //1970-1-1 0:0:0
		for (i = 0; i < len; i++) {
			switch (DateStr[i]) {
				case '0' ... '9':
					dateData = (void *)gBgramap[DateStr[i] - '0'].pdata; //点阵的地址
					fontadv = gBgramap[DateStr[i] - '0'].width;  //字体宽度
					penpos_t += gBgramap[DateStr[i] - '0'].width;    //水平偏移
					break;
				case '-':
					dateData = (void *)gBgramap[10].pdata;
					fontadv = gBgramap[10].width;
					penpos_t += gBgramap[10].width;
					break;
				case ' ':
					dateData = (void *)gBgramap[11].pdata;
					fontadv = gBgramap[11].width;
					penpos_t += gBgramap[11].width;
					break;
				case ':':
					dateData = (void *)gBgramap[12].pdata;
					fontadv = gBgramap[12].width;
					penpos_t += gBgramap[12].width;
					break;
				default:
					break;
			}
			for (j = 0; j < gBgramapHight; j++) {
				memcpy((void *)((uint32_t *)data + j*OSD_LETTER_NUM*OSD_REGION_WIDTH + penpos),
						(void *)((uint32_t *)dateData + j*fontadv), fontadv*sizeof(uint32_t));
			}
			penpos = penpos_t;
		}

		stISPOSDAsm.type = ISP_OSD_REG_PIC;
		stISPOSDAsm.stsinglepicAttr.chnOSDAttr.osd_type = IMP_ISP_PIC_ARGB_8888;
		stISPOSDAsm.stsinglepicAttr.chnOSDAttr.osd_group = 0;
		stISPOSDAsm.stsinglepicAttr.chnOSDAttr.osd_argb_type = IMP_ISP_ARGB_TYPE_BGRA;
		stISPOSDAsm.stsinglepicAttr.chnOSDAttr.osd_pixel_alpha_disable = IMPISP_TUNING_OPS_MODE_DISABLE;
		stISPOSDAsm.stsinglepicAttr.pic.pinum = isposdparam->phandles[1];
		stISPOSDAsm.stsinglepicAttr.pic.osd_enable = 1;
		stISPOSDAsm.stsinglepicAttr.pic.osd_left =  0;
		stISPOSDAsm.stsinglepicAttr.pic.osd_top = 0;
		stISPOSDAsm.stsinglepicAttr.pic.osd_width = OSD_REGION_WIDTH * OSD_LETTER_NUM;
		stISPOSDAsm.stsinglepicAttr.pic.osd_height = OSD_REGION_HEIGHT;
		stISPOSDAsm.stsinglepicAttr.pic.osd_image = (char*)data;
		stISPOSDAsm.stsinglepicAttr.pic.osd_stride = OSD_REGION_WIDTH * OSD_LETTER_NUM * 4;

		ret = IMP_OSD_SetRgnAttr_PicISP(0, isposdparam->phandles[1], &stISPOSDAsm);
		if (ret < 0) {
			printk("IMP_ISP_SetOSDAttr failed\n");
			return -1;
		}

		ret = IMP_OSD_ShowRgn_ISP(0, isposdparam->phandles[1], 1);
		if (ret < 0) {
			printk("IMP_OSD_ShowRgn_ISP failed\n");
			return -1;
		}

		/* Update Timestamp */
		mdelay(1000);
		last_date.second++;
		schedule();
	}

	printk("isposd Thread stopped\n");
	return 0;
}

int update_thread(void *p)
{
	IMP_Sample_OsdParam *pipuosdpam = (IMP_Sample_OsdParam*)p;
	/* generate time */
	int i = 0, j = 0;
	char DateStr[40] = { 0 };
	void *dateData = NULL;
	uint32_t *data = (void *)pipuosdpam->ptimestamps;
	Date current_date;
	Date last_date = {.year = 1970, .month = 1, .day = 1, .hour = 0, .minute = 0, .second = 0};
	memset(&current_date, 0, sizeof(Date));

	printk("ipuosd Thread started\n");
	while(!kthread_should_stop()){
		unsigned int len = 0;
		int fontadv = 0;
		int penpos_t = 0;
		int penpos = 0;

		calculate_dates(&current_date, &last_date);
		sprintf(DateStr, "%d-%02d-%02d %02d:%02d:%02d", current_date.year, current_date.month, current_date.day, current_date.hour, current_date.minute, current_date.second);
		//基于这个字符串，从点阵获取数据，memcpy到指定的内存
		len = strlen(DateStr);  //1970-1-1 0:0:0
		for (i = 0; i < len; i++) {
			switch (DateStr[i]) {
				case '0' ... '9':
					dateData = (void *)gBgramap[DateStr[i] - '0'].pdata; //点阵的地址
					fontadv = gBgramap[DateStr[i] - '0'].width;  //字体宽度
					penpos_t += gBgramap[DateStr[i] - '0'].width;    //水平偏移
					break;
				case '-':
					dateData = (void *)gBgramap[10].pdata;
					fontadv = gBgramap[10].width;
					penpos_t += gBgramap[10].width;
					break;
				case ' ':
					dateData = (void *)gBgramap[11].pdata;
					fontadv = gBgramap[11].width;
					penpos_t += gBgramap[11].width;
					break;
				case ':':
					dateData = (void *)gBgramap[12].pdata;
					fontadv = gBgramap[12].width;
					penpos_t += gBgramap[12].width;
					break;
				default:
					break;
			}
			for (j = 0; j < gBgramapHight; j++) {
				memcpy((void *)((uint32_t *)data + j*OSD_LETTER_NUM*OSD_REGION_WIDTH + penpos),
					   (void *)((uint32_t *)dateData + j*fontadv), fontadv*sizeof(uint32_t));
			}
			penpos = penpos_t;
		}
		mdelay(1000);
		last_date.second++;
		schedule();
	}
	printk("ipuosd Thread stopped\n");
	return 0;
}

int OSD_CreateRgn(int grpNum, uint32_t *timeStampData_ipu, int type)
{
	IMPRgnHandle *phandles;

	//次码流
	if(grpNum%3){
		if(type == TX_ISP_ISP_OSD_MODE)
		{
			printk("The secondary stream does not support the use of isposd!\n");
			return -1;
		}
		ipuosdparam.phandles = ipu_osd_create_rgn(grpNum, timeStampData_ipu);
//        ipuosdparam.type = type;
	} else{//主码流
		if(type == TX_ISP_ISP_OSD_MODE)
		{
			phandles = kmalloc(2 * sizeof(IMPRgnHandle),GFP_KERNEL);
			*phandles++ = isp_osd_create_rgn(grpNum, 0); //pic
			*phandles = isp_osd_create_rgn(grpNum, 0); //timeStamp
			isposdparam.phandles = --phandles;
//            isposdparam.type = type;
		} else if(type == TX_ISP_IPU_OSD_MODE){
			ipuosdparam.phandles = ipu_osd_create_rgn(grpNum, timeStampData_ipu);
//            ipuosdparam.type = type;
		}
	}

	return 0;
}

#ifndef __SENSOR_FAST_START_COMMON_H__
#define __SENSOR_FAST_START_COMMON_H__

#include <linux/kernel.h>

#define FIXED_INTERVAL_VIC_RESET 0

#define TX_ISP_FAST_START 1
#define TX_ISP_NRVBS_EN 1
#define TX_ISP_FRAME_BUFFER_IGNORE_NUM 0    // 顺序丢帧
#define TX_ISP_FRAME_BUFFER_IGNORE_CHOICE 0 // 选择性丢帧
#define FRAME_CHANNEL_DEF_WIDTH 1920
#define FRAME_CHANNEL_DEF_HEIGHT 1080

#define SENSOR_DAY_FPS 15
#define SENSOR_NIGHT_FPS 10

#define DEBUG_TTFF(a) printk("TTFF %s:%d %s:%d\n", __func__, __LINE__, (a==NULL? " " : a), 1000 * *(volatile u32 *)0xb0002078 * 16 / ((24 * 1000 * 1000) / 512))

char * get_sensor_name(void);
int get_sensor_i2c_addr(void);

#ifdef CONFIG_BUILT_IN_SENSOR_SETTING
uint8_t *get_sensor_setting(void);
int get_sensor_setting_len(void);
char *get_sensor_setting_date(void);
char *get_sensor_setting_md5(void);
#endif

#endif

#define VI_MAX 4
#include "tx-isp-fast.h"

struct add_sensor __attribute__((weak))  *sensors[8];
struct tx_isp_sensor_fast_attr  __attribute__((weak)) sensor0;
struct tx_isp_sensor_fast_attr  __attribute__((weak)) sensor1;
struct tx_isp_sensor_fast_attr  __attribute__((weak)) sensor2;
struct tx_isp_sensor_fast_attr  __attribute__((weak)) sensor3;
struct tx_isp_sensor_fast_attr  __attribute__((weak)) fast_sensors[VI_MAX];
struct tx_isp_callback_ops __attribute__((weak)) g_tx_isp_callback_ops;

void* __attribute__((weak)) setting_mem;
unsigned long __attribute__((weak)) rememAddrCh[4];
unsigned long __attribute__((weak)) virYAddr0;
unsigned long __attribute__((weak)) virYAddr1;
unsigned long __attribute__((weak)) kernel_save_ch0;
unsigned long __attribute__((weak)) kernel_save_ch1;
unsigned long __attribute__((weak)) frame_channel_width[12];
unsigned long __attribute__((weak)) frame_channel_height[12];
unsigned long __attribute__((weak)) frame_channel_nrvbs[12];
unsigned long __attribute__((weak)) direct_mode_save_kernel_ch1;
int __attribute__((weak)) sensor_wdr_mode;
int __attribute__((weak)) usr_wdr_mode;
int __attribute__((weak)) use_num_sensor;
int __attribute__((weak)) sensor_number;
int __attribute__((weak)) page_ch0_nums;
int __attribute__((weak)) page_ch1_nums;
int __attribute__((weak)) save_raw_nums;
unsigned long bufferSize0 = 0;
unsigned long bufferSize1 = 0;
char isp_buf_len[64 * VI_MAX] = {0};
char wdr_buf_len[64] = "";
char ivdc_buf_len[64] = "";
int vpuBs_size = 0;
int camera_mode = 0;
struct tx_isp_module *g_isp_module = NULL;
uint32_t g_start_sensor_inttime = 0;
uint32_t g_start_sensor_again = 0;
uint32_t g_start_isp_dgain = 0;
uint32_t g_start_isp_ev = 0;
uint32_t g_sec_start_isp_ev = 0;
uint32_t g_discard_frame = 0;
int g_debug_per_frame = 0;
int g_debug_frame_count = 0;
int g_sensor_expo_switch = 1;
char sub_sensor_w[255] = "";
char sub_sensor_h[255] = "";
struct miscdevice *g_f_mdev[12] = {0};
/* struct tx_isp_frame_sources *global_fs = NULL; */

int8_t  __attribute__((weak))   get_sensor_start_iq_index(int sensor_index)
{
	return 0;
}

uint8_t __attribute__((weak))   *get_sensor_setting(int sensor_index, int iq_index, int iq_section)
{
    return NULL;
}
void __attribute__((weak)) release_sensor_setting(int sensor_index, int iq_index, int iq_section)
{
    return;
}

int32_t __attribute__((weak)) get_sensor_section_num(int sensor_index, int iq_index)
{
	return 0;
}
uint32_t __attribute__((weak))  get_sensor_setting_len(int sensor_index, int iq_index, int iq_section)
{
    return 0;
}
void __attribute__((weak)) get_sensor_setting_md5(int sensor_index, int iq_index, int iq_section)
{
    return;
}
int __attribute__((weak)) sensor_setting_init(void)
{
    return 0;
}

int __attribute__((weak)) add_sensor_chose(void)
{
	return 0;
}


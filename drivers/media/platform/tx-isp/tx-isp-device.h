#ifndef __PRJ008_TX_ISP_DEVICE_H_
#define __PRJ008_TX_ISP_DEVICE_H_

#define MAX_TAG_ITER_SIZE		(64 * 1024)
#define SENSOR_SETTING_MAX_SIZE		(256 * 1024)
/*lzma src addr must 64 bytes align*/
#define CONFIG_TAG_LOADADDR             (0x3100000)
#define SENSOR_ATTR_LOADADDR		(0x3300000)
#define DECOMPRESS_SENSOR_LOADADDR	(0x3500000)
#define SENSOR_SETTING_ADDR		(DECOMPRESS_SENSOR_LOADADDR)

#define AE_TABLE_OFFSET			(1*1024*4 * 4)
#define AE_TABLE_ADDR			(0x3100000 + AE_TABLE_OFFSET)


#define MAX_TAG_ITER_SIZE_2SENSOR	(80 * 1024)
#define SENSOR0_SETTING_MAX_SIZE	(204 * 1024)
#define SENSOR1_SETTING_MAX_SIZE	(204 * 1024)
#define SENSOR0_SETTING_OFFSET		(MAX_TAG_ITER_SIZE_2SENSOR)
#define SENSOR0_SETTING_ADDR		(0x3800000 + SENSOR0_SETTING_OFFSET) /* Consistent with uboot */
#define SENSOR1_SETTING_OFFSET		(SENSOR0_SETTING_OFFSET + SENSOR0_SETTING_MAX_SIZE)
#define SENSOR1_SETTING_ADDR		(0x3800000 + SENSOR1_SETTING_OFFSET) /* Consistent with uboot */

#define AE0_TABLE_OFFSET		(1*1024*4 * 7)
#define AE0_TABLE_ADDR			(0x3800000 + AE0_TABLE_OFFSET)
#define AE1_TABLE_OFFSET		(1*1024*4 * 8)
#define AE1_TABLE_ADDR			(0x3800000 + AE1_TABLE_OFFSET)

enum {
	TX_ISP_DAY_BIN = 0,
	TX_ISP_NIGHT_BIN,
	TX_ISP_DAY_WDR_BIN,
	TX_ISP_MISC_BIN,
	TX_ISP_MAX_BIN,
};
const uint8_t *section_name[] = {"day","night","day-wdr","unknow"};
/*Maximum number of IQ files supported per sensor.*/
#define MAX_SENSOR_NUMS		4
#define MAX_IQ_NUMS		4
#define MAX_SECTIONS_NUMS	4
#define TAG_BLOCK_HEAD_ALIG	64

/*The definitions related to the header information of the sensor IQ file.*/
typedef struct {
	uint32_t size;				/*<*/
	uint32_t offset;			/*<*/
	uint32_t crc;				/*<*/
}Sections;
typedef union {
	Sections section[MAX_SECTIONS_NUMS];
	struct {
		uint32_t size;
		uint32_t offset;
		uint32_t crc;
	} attrs;
}VMAP;
typedef struct {
	uint8_t sections_cnt;
	VMAP vmap;
}IQ;
typedef struct {
	uint8_t iq_cnt;				/*<Number of IQ files corresponding to the sensor*/
	IQ iq_groups[MAX_IQ_NUMS];
}Sensor;
struct  __attribute__((aligned(1024))) sensor_calibration_info {
	uint32_t head;				/*<IQ file header identifier. SSET*/
	uint32_t version;			/*<IQ_CFG_VERSION*/
	uint32_t head_crc;			/*<Header structure verification*/
	uint32_t uncomp_fw_crc;			/*<Uncompressed data verification*/
	uint32_t comp_fw_crc;			/*<Compressed data verification*/
	uint32_t uncomp_len;			/*<Uncompressed data length*/
	uint32_t com_len;			/*<Compressed data length*/
	uint32_t sensor_cnt;			/*<Number of effective sensors.*/
	uint32_t per_max_size;			/*<Maximum length of each IQ file.*/
	Sensor sensor[MAX_SENSOR_NUMS];
};
struct sensor_calibration_head {
	struct sensor_calibration_info info;
	uint8_t data[0]; /* Just a way for get data */
};
#define SENSOR_CALIBRATION_HEAD_SIZE	sizeof(struct sensor_calibration_head)

/*To ISP drivers*/
struct tx_isp_sections{
	uint8_t *vaddr;
	uint32_t size;
	uint32_t crc;
	uint8_t ready_init;
};
typedef union{
	struct tx_isp_sections section[MAX_SECTIONS_NUMS];
	struct {
		uint8_t *vaddr;
		uint32_t size;
		uint32_t crc;
	} attrs;
}IQData;
struct tx_isp_iq_groups{
	uint8_t sections_cnt;
	IQData iq;
};
struct tx_isp_iq_attr {
	struct tx_isp_iq_groups iq_groups[MAX_IQ_NUMS];
};
int32_t get_sensor_section_num(int sensor_index, int iq_index);
int8_t get_sensor_start_iq_index(int sensor_index);
#endif

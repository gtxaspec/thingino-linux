#ifndef _INGENIC_VPU_H_
#define _INGENIC_VPU_H_

#include <linux/types.h>
//#define VPU_FILE_INPUT_TEST
#define NUM_MAX_ENCODER_CHN 16
#define NUM_MAX_CHAIN 256

typedef enum {
	PT_JPEG,
	PT_H264,
	PT_H265,
} VPUPayloadType;

typedef enum {
	FSTYPE_IDR		= 0,
	FSTYPE_LBASE	= 1,
	FSTYPE_SBASE	= 2,
	FSTYPE_ENHANCE	= 3,
} VPURefType;

typedef struct {
	VPUPayloadType			enType;			/* Encoding protocol type */
	unsigned int			picWidth;		/* Encode image width */
	unsigned int			picHeight;		/* Encode image height */
	unsigned int			fps;			/* Encode fps */
	unsigned int			Iqp;			/* I frame encode quality or jpeg qp */
	unsigned int			Pqp;			/* P frame encode quality */
	unsigned int			encPhyaddr;		/* Encode memory start address */
	unsigned int			strLen;			/* Stream memory length */
	bool					bEnableIvdc;	/* ISP VPU Direct Connect enable flag */
} VPUEncoderAttr;

struct tisp_to_enc_buf {
	uint32_t				index;
	int						chnum;
	uint32_t				type;
	uint32_t				memory;
	unsigned long			userptr;
	uint32_t				length;
};

/* Definition of frame image information */
typedef struct {
	int index;							/* Frame index */
	unsigned int width;					/* Frame width */
	unsigned int height;				/* Frame height */
	unsigned int size;					/* Frame memory size */
	unsigned int phyAddr;				/* Frame physical addr */
	unsigned int virAddr;				/* Frame virtual addr */
	unsigned int sensor_id;				/* Frame sensor id */
	long long int timestamp;			/* Frame timestamp */
	void *isp_priv;						/* ISP buf */
} FrameInfo;

#define KERN_ENC_MAGIC		'c'
#define IOCTL_KERN_ENC_STOP				_IO(KERN_ENC_MAGIC, 20)
#define IOCTL_KERN_ENC_GET_STREAM		_IOWR(KERN_ENC_MAGIC, 21, struct kern_enc_node)
#define IOCTL_KERN_ENC_RELEASE			_IOWR(KERN_ENC_MAGIC, 22, struct kern_enc_node)
#define IOCTL_KERN_ENC_GET_STATUS		_IOWR(KERN_ENC_MAGIC, 23, struct kern_enc_node)

#define NUM_MAX_STREAM 128

struct kern_enc_str_info {
	unsigned int str_offset;
	unsigned int str_len;
	long long int timestamp;
	int type;
};

struct kern_enc_node {
	int							chn_id; /* channel id */
	int							chn_status; /* channel status */
	int							type; /* payload type */
	unsigned int				output_addr; /* stream phy addr */
	unsigned int				output_len; /* stream len */
	unsigned int				output_cnt; /* frame count */
	struct kern_enc_str_info	str_info[NUM_MAX_STREAM]; /* stream info */
};
void _jz_osd_exit(void);
int VPU_Kern_Init(void);
int VPU_Kern_Exit(void);
int VPU_Encoder_CreateChn(int chnNum, const VPUEncoderAttr *chnAttr);
int VPU_Encoder_DestroyChn(int chnNum);
#ifdef VPU_FILE_INPUT_TEST
int VPU_Encoder_SendFrame(int chnNum, FrameInfo *frame);
#else
int VPU_Encoder_SendFrame(int chnNum);
#endif
int VPU_Encoder_GetChannelBufferSize(const VPUEncoderAttr *chnAttr);
int VPU_Encoder_SetVpuBsSize(unsigned int bsPhyaddr, unsigned int bsSize);
int VPU_Encoder_SetMultiSection(int enable);
int VPU_Encoder_SetJointAttr(const int *channels, int count);
#endif

#ifndef _JPEG_CORE_H_
#define _JPEG_CORE_H_

#include <linux/types.h>
#include "vpu_core.h"

typedef struct jpeg_instance jpeg_t;

struct jpeg_param {
	unsigned short width;
	unsigned short height;
	unsigned short y_stride;
	unsigned short c_stride;
	unsigned short qp;
};

struct jpeg_slice_info {
#define QMEM_LEN	(32)
	unsigned int qmem[QMEM_LEN];
	unsigned int jpeg_ctrl;
	unsigned int nmcu;
	unsigned int frm_info;
	unsigned int stride;
	unsigned int y_addr;
	unsigned int c_addr;
	unsigned int bs_addr;
	unsigned int pixel_num;
	unsigned int marker_en;
	unsigned int marker_num;
	unsigned int limit_bs_len;
};

struct jpeg_instance {
	struct jpeg_param param;
	struct jpeg_slice_info reg_info;
};

int jpeg_param_init(jpeg_t *inst, struct jpeg_param *param);
void jpeg_exit(jpeg_t *inst);
int jpeg_encode(jpeg_t *inst, struct vpu_io_info *input);

#endif //_JPEG_CORE_H_

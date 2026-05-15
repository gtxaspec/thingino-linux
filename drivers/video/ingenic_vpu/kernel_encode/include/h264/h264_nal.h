#ifndef _H264_NAL_H_
#define _H264_NAL_H_
#include "h264_bitstream.h"

enum nal_unit_type_e
{
	I264E_NAL_UNKNOWN     = 0,
	I264E_NAL_SLICE       = 1,
	I264E_NAL_SLICE_DPA   = 2,
	I264E_NAL_SLICE_DPB   = 3,
	I264E_NAL_SLICE_DPC   = 4,
	I264E_NAL_SLICE_IDR   = 5,    /* ref_idc != 0 */
	I264E_NAL_SEI         = 6,    /* ref_idc == 0 */
	I264E_NAL_SPS         = 7,
	I264E_NAL_PPS         = 8,
	I264E_NAL_AUD         = 9,
	I264E_NAL_FILLER      = 12,
	/* ref_idc == 0 for 6,9,10,11,12 */
};

enum nal_priority_e
{
	I264E_NAL_PRIORITY_DISPOSABLE = 0,
	I264E_NAL_PRIORITY_LOW        = 1,
	I264E_NAL_PRIORITY_HIGH       = 2,
	I264E_NAL_PRIORITY_HIGHEST    = 3,
};

typedef struct h264_nal
{
	int i_ref_idc;  /* nal_priority_e */
	int i_type;     /* nal_unit_type_e */
	unsigned int i_payload;
	unsigned char* p_payload;
} h264_nal;

typedef struct i264e_nal {
	h264_nal nal[16];
	unsigned int i_nal;
	unsigned char*  i_buffer;
	unsigned int    i_occupancy;
	unsigned int    i_allocSize;

	bool            i_bExtBuffer;
	unsigned char*  i_extraBuffer;
	unsigned int    i_extraOccupancy;

} i264e_nal_t;

int i264e_nal_init(i264e_nal_t *nal, void *addr, unsigned int len);
int i264e_nal_start(i264e_nal_t *nal, bs_t *bs, int i_type, int i_ref_idc);
int i264e_nal_start_segment(i264e_nal_t *nal, bs_t *bs, void *addr, unsigned int len, int is_final);
void i264e_nal_start_substream(i264e_nal_t *nal, void *addr, unsigned int len);
#endif //_H264_NAL_H_

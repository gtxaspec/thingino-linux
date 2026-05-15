#include "h264_nal.h"

int i264e_nal_init(i264e_nal_t *nal, void *addr, unsigned int len)
{
	memset(nal, 0, sizeof(i264e_nal_t));
	nal->i_buffer = addr;
	nal->i_allocSize = len;
	nal->i_bExtBuffer = 1;
	return 0;
}

int i264e_nal_start(i264e_nal_t *nal, bs_t *bs, int i_type, int i_ref_idc)
{
	int i = 0;
	static const char startCodePrefix[] = { 0, 0, 0, 1 };
	h264_nal *tmp_nal = NULL;
	unsigned char *out = NULL;
	unsigned int bytes = 0;
	unsigned int next_size = 0;
	const unsigned char* bpayload = bs->p_start;
	unsigned int payloadSize = bs_pos(bs) / 8;

	next_size = nal->i_occupancy + sizeof(startCodePrefix) + 2 + payloadSize + (payloadSize >> 1) + nal->i_extraOccupancy;
	if (next_size > nal->i_allocSize) {
		VPU_ERROR("stream buf is too small to fill the stream!\n");
		return VPU_BUFFULL;
	}

	//write start code
	out = nal->i_buffer + nal->i_occupancy;
	memcpy(out, startCodePrefix, 4);
	bytes += 4;
	out[bytes++] = ( 0x00 << 7 ) | ( i_ref_idc << 5 ) | ( i_type );

	//write *bs payload
	for (i = 0; i < payloadSize; i++)
	{
		if (i > 2 && !out[bytes - 2] && !out[bytes - 3] && out[bytes - 1] <= 0x03)
		{
			/* inject 0x03 to prevent emulating a start code */
			out[bytes] = out[bytes - 1];
			out[bytes - 1] = 0x03;
			bytes++;
		}

		out[bytes++] = bpayload[i];
	}

	//write bit stream
	if (nal->i_extraOccupancy)
	{
		/* these bytes were escaped by serializeSubstreams */
		for (i = 0; i < nal->i_extraOccupancy; i++)
		{
			if (i > 2 && !out[bytes - 2] && !out[bytes - 3] && out[bytes - 1] <= 0x03)
			{
				out[bytes] = out[bytes - 1];
				out[bytes - 1] = 0x03;
				bytes++;
			}

			out[bytes++] = nal->i_extraBuffer[i];
		}
		nal->i_extraOccupancy = 0;
	}


	nal->i_occupancy += bytes;

	tmp_nal = &nal->nal[nal->i_nal++];
	tmp_nal->i_ref_idc = i_ref_idc;
	tmp_nal->i_type = i_type;
	tmp_nal->i_payload = bytes;
	tmp_nal->p_payload = out;
	return 0;
}

void i264e_nal_start_substream(i264e_nal_t *nal, void *addr, unsigned int len)
{
	nal->i_extraBuffer = addr;
	nal->i_extraOccupancy = len;
}


#define H264E_RAW_STREAM_OFFSET   (4096 + 64 * 2)
int i264e_nal_start_segment(i264e_nal_t *nal, bs_t *bs, void *addr, unsigned int len, int is_final)
{
	h264_nal *nals = &nal->nal[nal->i_nal - 1];
	unsigned char *src = NULL;
	unsigned char *dst = NULL;
	unsigned char *end = NULL;
	unsigned char *tmp = NULL;

	if (len + nal->i_occupancy + 2 + H264E_RAW_STREAM_OFFSET > nal->i_allocSize) {
		VPU_ERROR("stream buf is too small to fill the segment stream!\n");
		return VPU_BUFFULL;
	}
	//When copying segmented bitstreams, free up the H265E_RAW_STREAM_SFFSET byte for the last copy
	memcpy(nal->i_buffer + H264E_RAW_STREAM_OFFSET + nal->i_occupancy, addr, len);
	nal->i_occupancy += len;

	if (is_final) {
		tmp = src = nals->p_payload + nals->i_payload + H264E_RAW_STREAM_OFFSET;
		dst = nals->p_payload + nals->i_payload;
		end = nal->i_buffer + nal->i_occupancy + H264E_RAW_STREAM_OFFSET;

		if( src < end ) *dst++ = *src++;
		if( src < end ) *dst++ = *src++;
		while( src < end )
		{
			if( src[0] <= 0x03 && !dst[-2] && !dst[-1] )
				*dst++ = 0x03;
			*dst++ = *src++;
		}
		if (!(*(dst - 1)))
			*dst++ = 0x03;

		nals->i_payload = (dst - nals->p_payload);
		nal->i_occupancy = (dst - nal->i_buffer);
	}

	return 0;

}

#include "vpu_common.h"
#include "h265_bitstream.h"
#include "h265_nal.h"

int serialize(NALList *list, NalUnitType nalUnitType, const Bitstream *bs)
{
	static const char startCodePrefix[] = { 0, 0, 0, 1 };

	int i = 0;
	unsigned int payloadSize = bs->m_byteOccupancy;
	unsigned int nextSize = 0;
	unsigned char *out = NULL;
	const unsigned char* bpayload = bs->m_fifo;
	unsigned int bytes = 0;
	h265_nal *nal = NULL;
	if (!bpayload)
		return -1;

	list->m_annexB = 1;
	nextSize = list->m_occupancy + sizeof(startCodePrefix) + 2 + payloadSize + (payloadSize >> 1) + list->m_extraOccupancy;
	if ((!list->m_bExtBuffer) && nextSize > list->m_allocSize)
	{
		unsigned char *temp = vpu_wrap_malloc(nextSize);
		if (temp)
		{
			memcpy(temp, list->m_buffer, list->m_occupancy);

			/* fixup existing payload pointers */
			for (i = 0; i < list->m_numNal; i++)
				list->m_nal[i].payload = temp + (list->m_nal[i].payload - list->m_buffer);

			vpu_wrap_free(list->m_buffer);
			list->m_buffer = temp;
			list->m_allocSize = nextSize;
		}
		else
		{
			VPU_ERROR("Unable to realloc access unit buffer\n");
			return VPU_BUFFULL;
		}
	} else if (list->m_bExtBuffer && nextSize > list->m_allocSize) {
		VPU_ERROR("stream buf is too small to fill the stream!\n");
		return VPU_BUFFULL;
	}

	out = list->m_buffer + list->m_occupancy;

#if 0
	if (!list->m_annexB)
	{
		/* Will write size later */
		bytes += 4;
	}
	else if (!list->m_numNal || nalUnitType == NAL_UNIT_VPS || nalUnitType == NAL_UNIT_SPS || nalUnitType == NAL_UNIT_PPS || nalUnitType == NAL_UNIT_UNSPECIFIED)
	{
		memcpy(out, startCodePrefix, 4);
		bytes += 4;
	}
	else
	{
		memcpy(out, startCodePrefix + 1, 3);
		bytes += 3;
	}
#else
	if (!list->m_annexB)
	{
		/* Will write size later */
		bytes += 4;
	}
	else
	{
		memcpy(out, startCodePrefix, 4);
		bytes += 4;
	}
#endif

	/* 16 bit NAL header:
	 * forbidden_zero_bit       1-bit
	 * nal_unit_type            6-bits
	 * nuh_reserved_zero_6bits  6-bits
	 * nuh_temporal_id_plus1    3-bits */
	out[bytes++] = (unsigned char)nalUnitType << 1;
	out[bytes++] = 1 + (nalUnitType == NAL_UNIT_CODED_SLICE_TSA_N);

	/* 7.4.1 ...
	 * Within the NAL unit, the following three-byte sequences shall not occur at
	 * any byte-aligned position:
	 *  - 0x000000
	 *  - 0x000001
	 *  - 0x000002 */
	for (i = 0; i < payloadSize; i++)
	{
		if (i > 2 && !out[bytes - 2] && !out[bytes - 3] && out[bytes - 1] <= 0x03 && nalUnitType != NAL_UNIT_UNSPECIFIED)
		{
			/* inject 0x03 to prevent emulating a start code */
			out[bytes] = out[bytes - 1];
			out[bytes - 1] = 0x03;
			bytes++;
		}

		out[bytes++] = bpayload[i];
	}

	VPU_CHECK(bytes <= 4 + 2 + payloadSize + (payloadSize >> 1), "NAL buffer overflow\n");

	if (list->m_extraOccupancy)
	{
		/* these bytes were escaped by serializeSubstreams */
		for (i = 0; i < list->m_extraOccupancy; i++)
		{
			if (i > 2 && !out[bytes - 2] && !out[bytes - 3] && out[bytes - 1] <= 0x03)
			{
				out[bytes] = out[bytes - 1];
				out[bytes - 1] = 0x03;
				bytes++;
			}

			out[bytes++] = list->m_extraBuffer[i];
		}
		list->m_extraOccupancy = 0;
	}

	/* 7.4.1.1
	 * ... when the last byte of the RBSP data is equal to 0x00 (which can
	 * only occur when the RBSP ends in a cabac_zero_word), a final byte equal
	 * to 0x03 is appended to the end of the data.  */
	if (!out[bytes - 1])
		out[bytes++] = 0x03;

	if (!list->m_annexB)
	{
		unsigned int dataSize = bytes - 4;
		out[0] = (unsigned char)(dataSize >> 24);
		out[1] = (unsigned char)(dataSize >> 16);
		out[2] = (unsigned char)(dataSize >> 8);
		out[3] = (unsigned char)dataSize;
	}

	list->m_occupancy += bytes;

	VPU_CHECK(list->m_numNal < (unsigned int)MAX_NAL_UNITS, "NAL count overflow\n");

	nal = &list->m_nal[list->m_numNal++];
	nal->type = nalUnitType;
	nal->sizeBytes = bytes;
	nal->payload = out;
	return 0;
}

/* concatenate and escape WPP sub-streams, return escaped row lengths.
 * These streams will be appended to the next serialized NAL */
void serializeSubstreams(NALList *list, void *addr, unsigned int len)
{
	list->m_extraBuffer = addr;
	list->m_extraOccupancy = len;
}

void initNal(NALList *list, void *extAddr, unsigned int len)
{
	memset(list, 0, sizeof(NALList));
	list->m_annexB = 1;
	list->m_bExtBuffer = 1;
	list->m_buffer = extAddr;
	list->m_allocSize = len;
}

#define H265E_RAW_STREAM_OFFSET   (4096 + 64 * 2)
int serializeSegment(NALList *list, void *addr, unsigned int len, int is_final)
{
	h265_nal *nal = &list->m_nal[list->m_numNal - 1];
	unsigned char *src = NULL;
	unsigned char *dst = NULL;
	unsigned char *end = NULL;
	unsigned char *tmp = NULL;

	if (len + list->m_occupancy + 2 + H265E_RAW_STREAM_OFFSET > list->m_allocSize) {
		VPU_ERROR("stream buf is too small to fill the segment stream!\n");
		return VPU_BUFFULL;
	}
	//When copying segmented bitstreams, free up the H265E_RAW_STREAM_SFFSET byte for the last copy
	memcpy(list->m_buffer + H265E_RAW_STREAM_OFFSET + list->m_occupancy, addr, len);
	list->m_occupancy += len;

	if (is_final) {
		tmp = src = nal->payload + nal->sizeBytes + H265E_RAW_STREAM_OFFSET;
		dst = nal->payload + nal->sizeBytes;
		end = list->m_buffer + list->m_occupancy + H265E_RAW_STREAM_OFFSET;

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

		nal->sizeBytes = (dst - nal->payload);
		list->m_occupancy = (dst - list->m_buffer);
	}

	return 0;
}

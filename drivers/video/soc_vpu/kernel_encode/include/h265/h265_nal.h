#ifndef _H265_NAL_H_
#define _H265_NAL_H_
#include "h265_bitstream.h"
#include "h265_slice.h"

#define MAX_NAL_UNITS 16
typedef struct h265_nal
{
	unsigned int type;        /* NalUnitType */
	unsigned int sizeBytes;   /* size in bytes */
	unsigned char* payload;
} h265_nal;

typedef struct NALList
{
	h265_nal    m_nal[16];
	unsigned int    m_numNal;

	unsigned char*    m_buffer;
	unsigned int    m_occupancy;
	unsigned int    m_allocSize;

	bool        m_bExtBuffer;
	unsigned char*    m_extraBuffer;
	unsigned int    m_extraOccupancy;
	unsigned int    m_extraAllocSize;
	bool        m_annexB;
} NALList;
int serialize(NALList *list, NalUnitType nalUnitType, const Bitstream *bs);
int serializeSegment(NALList *list, void *addr, unsigned int len, int is_final);
void serializeSubstreams(NALList *list, void *addr, unsigned int len);
void initNal(NALList *list, void *extAddr, unsigned int len);

#endif //_H265_NAL_H_

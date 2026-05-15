#ifndef _H265_BITSTREAM_H_
#define _H265_BITSTREAM_H_

typedef struct Bitstream {
	unsigned char *m_fifo;
	unsigned int m_byteAlloc;
	unsigned int m_byteOccupancy;
	unsigned int m_partialByteBits;
	unsigned char  m_partialByte;
} Bitstream;

void resetBits(Bitstream *bs);
void initBits(Bitstream *bs);
void freeBits(Bitstream *bs);
void writeByteAlignment(Bitstream *bs);

void WRITE_CODE(Bitstream *bs, unsigned int code, unsigned int length, const char *str);
void WRITE_UVLC(Bitstream *bs, unsigned int code,                  const char *str);
void WRITE_SVLC(Bitstream *bs, int  code,                  const char *str);
void WRITE_FLAG(Bitstream *bs, bool flag,                      const char *str);


#endif // _H265_BITSTREAM_H_

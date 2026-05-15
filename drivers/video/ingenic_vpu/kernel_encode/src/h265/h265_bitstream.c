#include "vpu_common.h"
#include "h265_bitstream.h"
#include <linux/string.h>

#define MIN_FIFO_SIZE 1000
#define CLZ(id, x)            id = (unsigned long)__builtin_clz(x) ^ 31
#define CTZ(id, x)            id = (unsigned long)__builtin_ctz(x)

void resetBits(Bitstream *bs)
{
	bs->m_partialByteBits = 0;
	bs->m_byteOccupancy = 0;
	bs->m_partialByte = 0;
}

void copyBits(Bitstream* src, Bitstream* dst)
{
	dst->m_partialByteBits = src->m_partialByteBits;
	dst->m_byteOccupancy = src->m_byteOccupancy;
	dst->m_partialByte = src->m_partialByte;
}

void initBits(Bitstream *bs)
{
	bs->m_fifo = vpu_wrap_malloc(MIN_FIFO_SIZE);
	bs->m_byteAlloc = MIN_FIFO_SIZE;
	resetBits(bs);
}

void freeBits(Bitstream *bs)
{
	vpu_wrap_free(bs->m_fifo);
	resetBits(bs);
}

void bs_push_back(Bitstream *bs, unsigned char val)
{
	unsigned char *temp = NULL;
	if (!bs->m_fifo)
		return;

	if (bs->m_byteOccupancy >= bs->m_byteAlloc)
	{
		/** reallocate buffer with doubled size */
		temp = vpu_wrap_malloc(bs->m_byteAlloc * 2);
		if (temp)
		{
			memcpy(temp, bs->m_fifo, bs->m_byteOccupancy);
			vpu_wrap_free(bs->m_fifo);
			bs->m_fifo = temp;
			bs->m_byteAlloc *= 2;
		}
		else
		{
			VPU_ERROR("Unable to realloc bitstream buffer");
			return;
		}
	}
	bs->m_fifo[bs->m_byteOccupancy++] = val;
}

void bs_write(Bitstream *bs, unsigned int val, unsigned int numBits)
{
	unsigned int totalPartialBits = bs->m_partialByteBits + numBits;
	unsigned int nextPartialBits = totalPartialBits & 7;
	unsigned char  nextHeldByte = val << (8 - nextPartialBits);
	unsigned int writeBytes = totalPartialBits >> 3;
	unsigned int topword = 0;
	unsigned int write_bits = 0;

	VPU_CHECK(numBits <= 32, "numBits out of range\n");
	VPU_CHECK(numBits == 32 || ((val & (~0u << numBits)) == 0), "numBits & val out of range\n");

	if (writeBytes)
	{
		/* topword aligns m_partialByte with the msb of val */
		topword = (numBits - nextPartialBits) & ~7;
		write_bits = (bs->m_partialByte << topword) | (val >> nextPartialBits);

		switch (writeBytes)
		{
			case 4: bs_push_back(bs, write_bits >> 24);  // fall-through
			case 3: bs_push_back(bs, write_bits >> 16);  // fall-through
			case 2: bs_push_back(bs, write_bits >> 8);   // fall-through
			case 1: bs_push_back(bs, write_bits);
		}

		bs->m_partialByte = nextHeldByte;
		bs->m_partialByteBits = nextPartialBits;
	}
	else
	{
		bs->m_partialByte |= nextHeldByte;
		bs->m_partialByteBits = nextPartialBits;
	}
}

void writeByte(Bitstream *bs, unsigned int val)
{
	// Only CABAC will call writeByte, the fifo must be byte aligned
	VPU_CHECK(!bs->m_partialByteBits, "expecting m_partialByteBits = 0\n");

	bs_push_back(bs, val);
}

void writeAlignOne(Bitstream *bs)
{
	unsigned int numBits = (8 - bs->m_partialByteBits) & 0x7;

	bs_write(bs, (1 << numBits) - 1, numBits);
}

void writeAlignZero(Bitstream *bs)
{
	if (bs->m_partialByteBits)
	{
		bs_push_back(bs, bs->m_partialByte);
		bs->m_partialByte = 0;
		bs->m_partialByteBits = 0;
	}
}

void writeByteAlignment(Bitstream *bs)
{
	bs_write(bs, 1, 1);
	writeAlignZero(bs);
}

void writeUvlc(Bitstream *bs, unsigned int code)
{
	unsigned long idx;
	unsigned int length;
	++code;

	VPU_CHECK(code, "writing -1 code, will cause infinite loop\n");
	CLZ(idx, code);
	length = (unsigned int)idx * 2 + 1;

	// Take care of cases where length > 32
	bs_write(bs, 0, length >> 1);
	bs_write(bs, code, (length + 1) >> 1);
}

void writeSvlc(Bitstream *bs, int code)
{
	unsigned int ucode = (code <= 0) ? -code << 1 : (code << 1) - 1;
	writeUvlc(bs, ucode);
}

void WRITE_CODE(Bitstream *bs, unsigned int code, unsigned int length, const char *str) { bs_write(bs, code, length); }
void WRITE_UVLC(Bitstream *bs, unsigned int code,                  const char *str) { writeUvlc(bs, code); }
void WRITE_SVLC(Bitstream *bs, int  code,                  const char *str) { writeSvlc(bs, code); }
void WRITE_FLAG(Bitstream *bs, bool flag,                      const char *str) { bs_write(bs, flag, 1); }

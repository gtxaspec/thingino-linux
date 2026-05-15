#ifndef _H264_BITSTREAM_H_
#define _H264_BITSTREAM_H_
#include "vpu_common.h"

#define WORD_SIZE 4
typedef struct bs_s
{
	unsigned char *p_start;
	unsigned char *p;
	unsigned char *p_end;

	unsigned int cur_bits;
	int     i_left;    /* i_count number of available bits */
	int     i_bits_encoded; /* RD only */
} bs_t;

static inline uint32_t endian_fix32( uint32_t x )
{
	return (x<<24) + ((x<<8)&0xff0000) + ((x>>8)&0xff00) + (x>>24);
}

static inline uint64_t endian_fix64( uint64_t x )
{
	return endian_fix32(x>>32) + ((uint64_t)endian_fix32(x)<<32);
}

static inline int endian_fix( int x )
{
	return WORD_SIZE == 8 ? endian_fix64(x) : endian_fix32(x);
}

static inline void bs_init( bs_t *s, void *p_data, int i_data )
{
	int offset = ((int)p_data & 3);
	s->p       = s->p_start = (unsigned char*)p_data - offset;
	s->p_end   = (unsigned char*)p_data + i_data;
	s->i_left  = (WORD_SIZE - offset)*8;
	s->cur_bits = endian_fix32( C_M32(s->p) );
	s->cur_bits >>= (4-offset)*8;
}
static inline int bs_pos( bs_t *s )
{
	return( 8 * (s->p - s->p_start) + (WORD_SIZE*8) - s->i_left );
}

/* Write the rest of cur_bits to the bitstream; results in a bitstream no longer 32-bit aligned. */
static inline void bs_flush( bs_t *s )
{
	C_M32( s->p ) = endian_fix32( s->cur_bits << (s->i_left&31) );
	s->p += WORD_SIZE - (s->i_left >> 3);
	s->i_left = WORD_SIZE*8;
}
/* The inverse of bs_flush: prepare the bitstream to be written to again. */
static inline void bs_realign( bs_t *s )
{
	int offset = ((int)s->p & 3);
	if( offset )
	{
		s->p       = (unsigned char*)s->p - offset;
		s->i_left  = (WORD_SIZE - offset)*8;
		s->cur_bits = endian_fix32( C_M32(s->p) );
		s->cur_bits >>= (4-offset)*8;
	}
}

static inline void bs_write( bs_t *s, int i_count, uint32_t i_bits )
{
	if( WORD_SIZE == 8 )
	{
		s->cur_bits = (s->cur_bits << i_count) | i_bits;
		s->i_left -= i_count;
		if( s->i_left <= 32 )
		{
#ifdef WORDS_BIGENDIAN
			C_M32( s->p ) = s->cur_bits >> (32 - s->i_left);
#else
			C_M32( s->p ) = endian_fix( s->cur_bits << s->i_left );
#endif
			s->i_left += 32;
			s->p += 4;
		}
	}
	else
	{
		if( i_count < s->i_left )
		{
			s->cur_bits = (s->cur_bits << i_count) | i_bits;
			s->i_left -= i_count;
		}
		else
		{
			i_count -= s->i_left;
			s->cur_bits = (s->cur_bits << s->i_left) | (i_bits >> i_count);
			C_M32( s->p ) = endian_fix( s->cur_bits );
			s->p += 4;
			s->cur_bits = i_bits;
			s->i_left = 32 - i_count;
		}
	}
}

/* Special case to eliminate branch in normal bs_write. */
/* Golomb never writes an even-size code, so this is only used in slice headers. */
static inline void bs_write32( bs_t *s, uint32_t i_bits )
{
	bs_write( s, 16, i_bits >> 16 );
	bs_write( s, 16, i_bits );
}

static inline void bs_write1( bs_t *s, uint32_t i_bit )
{
	s->cur_bits <<= 1;
	s->cur_bits |= i_bit;
	s->i_left--;
	if( s->i_left == WORD_SIZE*8-32 )
	{
		C_M32( s->p ) = endian_fix32( s->cur_bits );
		s->p += 4;
		s->i_left = WORD_SIZE*8;
	}
}

static inline void bs_align_0( bs_t *s )
{
	bs_write( s, s->i_left&7, 0 );
	bs_flush( s );
}
static inline void bs_align_1( bs_t *s )
{
	bs_write( s, s->i_left&7, (1 << (s->i_left&7)) - 1 );
	bs_flush( s );
}
static inline void bs_align_10( bs_t *s )
{
	if( s->i_left&7 )
		bs_write( s, s->i_left&7, 1 << ( (s->i_left&7) - 1 ) );
}

/* golomb functions */

static const unsigned char i264e_ue_size_tab[256] =
{
	1, 1, 3, 3, 5, 5, 5, 5, 7, 7, 7, 7, 7, 7, 7, 7,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
	11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
	13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
	13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
	13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
	13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
	15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
};

static inline void bs_write_ue_big( bs_t *s, unsigned int val )
{
	int size = 0;
	int tmp = ++val;
	if( tmp >= 0x10000 )
	{
		size = 32;
		tmp >>= 16;
	}
	if( tmp >= 0x100 )
	{
		size += 16;
		tmp >>= 8;
	}
	size += i264e_ue_size_tab[tmp];
	bs_write( s, size>>1, 0 );
	bs_write( s, (size>>1)+1, val );
}

/* Only works on values under 255. */
static inline void bs_write_ue( bs_t *s, int val )
{
	bs_write( s, i264e_ue_size_tab[val+1], val+1 );
}

static inline void bs_write_se( bs_t *s, int val )
{
	int size = 0;
	/* Faster than (val <= 0 ? -val*2+1 : val*2) */
	/* 4 instructions on x86, 3 on ARM */
	int tmp = 1 - val*2;
	if( tmp < 0 ) tmp = val*2;
	val = tmp;

	if( tmp >= 0x100 )
	{
		size = 16;
		tmp >>= 8;
	}
	size += i264e_ue_size_tab[tmp];
	bs_write( s, size, val );
}

static inline void bs_write_te( bs_t *s, int x, int val )
{
	if( x == 1 )
		bs_write1( s, 1^val );
	else //if( x > 1 )
		bs_write_ue( s, val );
}

static inline void bs_rbsp_trailing( bs_t *s )
{
	bs_write1( s, 1 );
	bs_write( s, s->i_left&7, 0  );
}

static inline int bs_size_ue( unsigned int val )
{
	return i264e_ue_size_tab[val+1];
}

static inline int bs_size_ue_big( unsigned int val )
{
	if( val < 255 )
		return i264e_ue_size_tab[val+1];
	else
		return i264e_ue_size_tab[(val+1)>>8] + 16;
}

static inline int bs_size_se( int val )
{
	int tmp = 1 - val*2;
	if( tmp < 0 ) tmp = val*2;
	if( tmp < 256 )
		return i264e_ue_size_tab[tmp];
	else
		return i264e_ue_size_tab[tmp>>8]+16;
}

static inline int bs_size_te( int x, int val )
{
	if( x == 1 )
		return 1;
	else //if( x > 1 )
		return i264e_ue_size_tab[val+1];
}

#endif

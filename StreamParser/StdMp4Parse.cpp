#include "StdMp4Parse.h"

#define VIDOBJLAY_START_CODE_MASK 0x0000000f
#define VIDOBJLAY_START_CODE 0x00000120 
#define VIDOBJLAY_AR_EXTPAR 15
#define VIDOBJLAY_SHAPE_BINARY_ONLY 2
#define VIDOBJLAY_SHAPE_RECTANGULAR 0
#define VIDOBJLAY_SHAPE_GRAYSCALE		3
#define uint32_t unsigned int
#define READ_MARKER() BitstreamSkip(&bs, 1)
typedef struct
{
	uint32_t bufa;
	uint32_t bufb;
	uint32_t buf;
	uint32_t pos;
	uint32_t *tail;
	uint32_t *start;
	uint32_t length;
}
Bitstream;

#        define BSWAP(a) \
                ((a) = (((a) & 0xff) << 24)  | (((a) & 0xff00) << 8) | \
                       (((a) >> 8) & 0xff00) | (((a) >> 24) & 0xff))

static void __inline
BitstreamSkip(Bitstream * const bs,
			  const uint32_t bits)
{
	bs->pos += bits;

	if (bs->pos >= 32) {
		uint32_t tmp;

		bs->bufa = bs->bufb;
		tmp = *((uint32_t *) bs->tail + 2);
		BSWAP(tmp);
		bs->bufb = tmp;
		bs->tail++;
		bs->pos -= 32;
	}
}

static uint32_t __inline
BitstreamShowBits(Bitstream * const bs,
				  const uint32_t bits)
{
	int nbit = (bits + bs->pos) - 32;

	if (nbit > 0) {
		return ((bs->bufa & (0xffffffff >> bs->pos)) << nbit) | (bs->
																 bufb >> (32 -
																		  nbit));
	} else {
		return (bs->bufa & (0xffffffff >> bs->pos)) >> (32 - bs->pos - bits);
	}
}

static uint32_t __inline
BitstreamGetBits(Bitstream * const bs,
				 const uint32_t n)
{
	uint32_t ret = BitstreamShowBits(bs, n);

	BitstreamSkip(bs, n);
	return ret;
}


/* read single bit from bitstream */

static uint32_t __inline
BitstreamGetBit(Bitstream * const bs)
{
	return BitstreamGetBits(bs, 1);
}

static void __inline
BitstreamInit(Bitstream * const bs,
			  void *const bitstream,
			  uint32_t length)
{
	uint32_t tmp;

	bs->start = bs->tail = (uint32_t *) bitstream;

	tmp = *(uint32_t *) bitstream;

	BSWAP(tmp);

	bs->bufa = tmp;

	tmp = *((uint32_t *) bitstream + 1);

	BSWAP(tmp);

	bs->bufb = tmp;

	bs->buf = 0;
	bs->pos = 0;
	bs->length = length;
}


inline int log2bin(unsigned int value) 
{
	int n = 0;
	while (value) {
		value>>=1;
		n++;
	}
	return n;
}

#define MMAX(a,b) ((a)>(b)?(a):(b))

int ParseStdMp4(unsigned char* pBuf, int len, int* width, int* height)
{
	int iRet = -1;
	if (pBuf == 0 || width == 0 || height == 0)
	{
		return iRet;
	}

	Bitstream bs;
	BitstreamInit(&bs, pBuf, len);

// 		BitstreamSkip(&bs, 32); /* video_object_layer_start_code */

	BitstreamSkip(&bs, 1); /* random_accessible_vol */

	BitstreamSkip(&bs, 8);

	int vol_ver_id = 0;
	if (BitstreamGetBit(&bs)) /* is_object_layer_identifier */
	{
		vol_ver_id = BitstreamGetBits(&bs, 4); /* video_object_layer_verid */
		BitstreamSkip(&bs, 3); /* video_object_layer_priority */
	} 
	else 
	{
		vol_ver_id = 1;
	}

	if (BitstreamGetBits(&bs, 4) == VIDOBJLAY_AR_EXTPAR) /* aspect_ratio_info */
	{
		BitstreamSkip(&bs, 8); /* par_width */
		BitstreamSkip(&bs, 8); /* par_height */
	}

	if (BitstreamGetBit(&bs)) /* vol_control_parameters */
	{
		BitstreamSkip(&bs, 2); /* chroma_format */
		int low_delay = (unsigned char)BitstreamGetBit(&bs); /* low_delay */
		if (BitstreamGetBit(&bs)) /* vbv_parameters */
		{
			 BitstreamSkip(&bs, 15); /* first_half_bitrate */
			 READ_MARKER();
			 BitstreamSkip(&bs, 15); /* latter_half_bitrate */
			 READ_MARKER();
			 BitstreamSkip(&bs, 15); /* first_half_vbv_buffer_size */
			 READ_MARKER();
			 BitstreamSkip(&bs, 3); /* latter_half_vbv_buffer_size */
			 BitstreamSkip(&bs, 11); /* first_half_vbv_occupancy */
			 READ_MARKER();
			 BitstreamSkip(&bs, 15); /* latter_half_vbv_occupancy */
			 READ_MARKER();
		}
	}

	int shape = BitstreamGetBits(&bs, 2); /* video_object_layer_shape */

	if (shape == VIDOBJLAY_SHAPE_GRAYSCALE && vol_ver_id != 1)
	{
		BitstreamSkip(&bs, 4); /* video_object_layer_shape_extension */
	}

	READ_MARKER();

	int time_increment_resolution = BitstreamGetBits(&bs, 16); /* vop_time_increment_resolution */

	int time_inc_bits;
	if (time_increment_resolution > 0) 
	{
		time_inc_bits = log2bin(time_increment_resolution-1);
	}
	else
	{
		time_inc_bits = 1;
	}

	READ_MARKER();

	if (BitstreamGetBit(&bs)) /* fixed_vop_rate */
	{
		BitstreamSkip(&bs, time_inc_bits); /* fixed_vop_time_increment */
	}

	if (shape != VIDOBJLAY_SHAPE_BINARY_ONLY)
	{
		if (shape == VIDOBJLAY_SHAPE_RECTANGULAR)
		{
			 READ_MARKER();
			 *width = BitstreamGetBits(&bs, 13); /* video_object_layer_width */
			 READ_MARKER();
			 *height = BitstreamGetBits(&bs, 13); /* video_object_layer_height */
			 READ_MARKER();
			 iRet = 0;
		}
	}

	return iRet;
}

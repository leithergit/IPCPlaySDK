
/*************************************************************************/
/*                                                                       */
/*                       MPEG4 BitStream解析相关                         */
/*								winton			                         */
/*							  2008-1-30                                  */
/*                                                                       */
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "decSPS.h"

typedef struct
{
    unsigned int bufa;
    unsigned int bufb;
    unsigned int buf;
    unsigned int pos;
    unsigned int *tail;
    unsigned int *start;
    unsigned int length;
}
Bitstream;

unsigned int get_int(void *ptr)
{
    unsigned char *buffer = (unsigned char *)ptr;
    return (*buffer) | ((*(buffer + 1)) << 8)| ((*(buffer + 2)) << 16)| ((*(buffer + 3)) << 24);
}

int MPEG4_BSWAP(void *v)
{
    unsigned int x =  *(unsigned int *) v;

    *(int*)v = (x&0xFF)<<24|(x&0xFF00)<<8|(x&0xFF0000)>>8|(x&0xFF000000)>>24;
    return 0;
}

void MPEG4_BitstreamInit(Bitstream *  bs, void * bitstream, unsigned int length)
{
    unsigned int tmp;

    bs->start = bs->tail = (unsigned int *) bitstream;

    //tmp = *(unsigned int *) bitstream;
    tmp = get_int(bitstream);

    MPEG4_BSWAP(&tmp);

    bs->bufa = tmp;

    //tmp = *((unsigned int *) bitstream + 1);
    tmp = get_int((void*)((unsigned long)bitstream + sizeof(int)));
    MPEG4_BSWAP(&tmp);
    bs->bufb = tmp;

    bs->buf = 0;
    bs->pos = 0;
    bs->length = length;
}

unsigned int MPEG4_BitstreamShowBits(Bitstream *  bs, unsigned int bits)
{
    int nbit = (bits + bs->pos) - 32;

    if (nbit > 0)
    {
        return ((bs->bufa & (0xffffffff >> bs->pos)) << nbit) | (bs->bufb >> (32 - nbit));
    }
    else
    {
        return (bs->bufa & (0xffffffff >> bs->pos)) >> (32 - bs->pos - bits);
    }
}

void MPEG4_BitstreamSkip(Bitstream * bs, unsigned int bits)
{
    bs->pos += bits;

    if (bs->pos >= 32)
    {
        unsigned int tmp;

        bs->bufa = bs->bufb;
        //tmp = *((unsigned int *) bs->tail + 2);
        tmp = get_int((void *)((unsigned int *) bs->tail + 2));
        MPEG4_BSWAP(&tmp);
        bs->bufb = tmp;
        bs->tail++;
        bs->pos -= 32;
    }

}

void MPEG4_BitstreamByteAlign(Bitstream * bs)
{
    unsigned int remainder = bs->pos % 8;

    if (remainder)
    {
        MPEG4_BitstreamSkip(bs, 8 - remainder);
    }
}

unsigned int MPEG4_BitstreamGetBits(Bitstream * const bs,unsigned int n)
{
    unsigned int ret = MPEG4_BitstreamShowBits(bs, n);

    MPEG4_BitstreamSkip(bs, n);

    return ret;
}

unsigned int MPEG4_BitstreamGetBit(Bitstream * const bs)
{
    return MPEG4_BitstreamGetBits(bs, 1);
}


unsigned int MPEG4_log2bin(unsigned int value)
{
    int n = 0;

    while (value)
    {
        value >>= 1;
        n++;
    }
    return n;
}

unsigned int MPEG4_BitstreamPos(Bitstream * bs)
{
    return((unsigned int)(8*((unsigned long)bs->tail - (unsigned long)bs->start) + bs->pos));
}

/*************************************************************************/
/*                                                                       */
/*                         H264 BitStream解析相关                        */
/*								winton			                         */
/*							  2008-1-30                                  */
/*                                                                       */
/*************************************************************************/


typedef struct GetBitContext
{
    unsigned char *buffer;
    unsigned char *buffer_end;
    int index;
    int size_in_bits;
} GetBitContext;

unsigned char _ff_golomb_vlc_len[32]=
{
    0, 9, 7, 7, 5, 5, 5, 5, 3, 3, 3, 3, 3, 3, 3, 3,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

unsigned char ff_log2_table[128]=
{
    0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
};


unsigned char _ff_ue_golomb_vlc_code[256]=
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,
    7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
};

char ff_se_golomb_vlc_code_[256]=
{
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8, -8,  9, -9, 10,-10, 11,-11, 12,-12, 13,-13, 14,-14, 15,-15,
    4,  4,  4,  4, -4, -4, -4, -4,  5,  5,  5,  5, -5, -5, -5, -5,  6,  6,  6,  6, -6, -6, -6, -6,  7,  7,  7,  7, -7, -7, -7, -7,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

int H264_unaligned32_be(void *v)
{
    //unsigned int x =  *(unsigned int *) v;
    unsigned int x =  get_int(v);
    return  (x&0xFF)<<24|(x&0xFF00)<<8|(x&0xFF0000)>>8|(x&0xFF000000)>>24;
}

void H264_init_get_bits(GetBitContext *s, unsigned char *buffer, int bit_size)
{
    int buffer_size= (bit_size+7)>>3;

    s->buffer= buffer;
    s->size_in_bits= bit_size;
    s->buffer_end= buffer + buffer_size;
    s->index=0;
    {
        int re_index= s->index;
        int re_cache=  H264_unaligned32_be( ((unsigned char *)s->buffer)+(re_index>>3) ) << (re_index&0x07);

        s->index= re_index;
    }
}

void H264_skip_bits(GetBitContext *s, int n)
{
    s->index += n;
}

void H264_skip_one_bits(GetBitContext *s)
{
    H264_skip_bits(s, 1);
}

unsigned int H264_get_one_bit(GetBitContext *s)
{

    int index = s->index;
    unsigned char result = s->buffer[index>>3];
    result <<= (index&0x07);
    result >>= 7;
    index++;
    s->index = index;
    return result;
}
int unaligned32_be(void *v)
{
    unsigned int x =  *(unsigned int *) v;

    return  ((x&0xff)<<24)|((x&0xff00)<<8)|((x&0xff0000)>>8)|((x&0xff000000)>>24);
}



unsigned int H264_get_bits(GetBitContext *s, int n)
{

    int tmp;
    int re_index= s->index;
    int re_cache= 0;
    re_cache= unaligned32_be( ((unsigned char *)(s)->buffer)+(re_index>>3) ) << (re_index&0x07);
    tmp= (((unsigned long)(re_cache))>>(32-(n)));
    re_index += n;
    s->index= re_index;
    return tmp;

}
unsigned int get_bits_long(GetBitContext *s, int n)
{
    if(n<=17)
    {
        return H264_get_bits(s, n);
    }
    else
    {
        int ret= H264_get_bits(s, 16) << (n-16);
        return ret | H264_get_bits(s, n-16);
    }
}
int H264_av_log2(unsigned int v)
{
    int n = 0;
    if (v & 0xffff0000)
    {
        v >>= 16;
        n += 16;
        if(v & 0xff00)
        {
            v >>= 8;
            n += 8;
        }
    }
    else if (v & 0xff00)
    {
        v >>= 8;
        n += 8;
    }
    n += ff_log2_table[v>>1];

    return n;
}

int H264_get_ue_golomb(GetBitContext *gb)
{
    unsigned int	buf;
    int				log;
    int				re_index = gb->index;
    int				re_cache = 0;
    re_cache		= H264_unaligned32_be( ((unsigned char *)(gb)->buffer)+(re_index>>3) ) << (re_index&0x07);
    buf				= (unsigned int)re_cache;

    if(buf >= (1<<27))
    {
        buf >>= 32 - 9;
        re_index += _ff_golomb_vlc_len[buf>>4];

        gb->index = re_index;

        if(buf<256)
        {
            return _ff_ue_golomb_vlc_code[buf];
        }
        else
        {
            return 0;
        }
    }
    else
    {
        log = 2*H264_av_log2(buf) - 31;
        buf>>= log;
        buf--;
        re_index += (32 - log);
        gb->index= re_index;
        return buf;
    }
}


int H264_get_se_golomb(GetBitContext *gb)
{
    unsigned int	buf;
    int				log;
    int				re_index= gb->index;
    int				re_cache= 0;
    re_cache		= H264_unaligned32_be( ((unsigned char *)(gb)->buffer)+(re_index>>3) ) << (re_index&0x07);
    buf				= re_cache;

    if(buf >= (1<<27))
    {
        buf >>= 32 - 9;
        re_index += _ff_golomb_vlc_len[buf>>4];
        gb->index= re_index;
        if(buf<256)
        {
            return ff_se_golomb_vlc_code_[buf];
        }
        else
        {
            return 0;
        }
    }
    else
    {
        log = 2*H264_av_log2(buf) - 31;
        buf >>= log;
        re_index += (32 - log);
        gb->index = re_index;

        if(buf&1)
            buf = -(buf>>1);
        else
            buf = (buf>>1);

        return buf;
    }
}
/*************************************************************************/
/*                                                                       */
/*                         MPEG4 Analyse function                        */
/*								winton			                         */
/*							  2008-1-30                                  */
/*                                                                       */
/*************************************************************************/
int Mpeg4_Analyse(unsigned char* pBuf,int nSize,int* nWidth,int* nHeight,int*framerate)
{
    Bitstream		bs;
    int				time_increment				= 0;
    int				frame_type					= 0;
    int				shape						= 0;
    int				time_inc_bits				= 0;
    unsigned int	vol_ver_id					= 0;
    unsigned int	time_increment_resolution	= 0;
    unsigned int	coding_type					= 0;
    unsigned int	start_code					= 0;
    unsigned int	time_incr					= 0;

    MPEG4_BitstreamInit(&bs, pBuf, nSize);

    do
    {
        MPEG4_BitstreamByteAlign(&bs);
        start_code = MPEG4_BitstreamShowBits(&bs, 32);

        if (start_code == 0x01b0)
        {
            MPEG4_BitstreamSkip(&bs, 40);
        }
        else if (start_code == 0x01b1)
        {
            MPEG4_BitstreamSkip(&bs, 32);
        }
        else if (start_code == 0x01b5)
        {
            MPEG4_BitstreamSkip(&bs, 32);
            if (MPEG4_BitstreamGetBit(&bs))
            {
                vol_ver_id = MPEG4_BitstreamGetBits(&bs, 4);
                MPEG4_BitstreamSkip(&bs, 3);
            }
            else
            {
                vol_ver_id = 1;
            }

            if (MPEG4_BitstreamShowBits(&bs, 4) != 1)
            {
                return -1;
            }
            MPEG4_BitstreamSkip(&bs, 4);

            if (MPEG4_BitstreamGetBit(&bs))
            {
                MPEG4_BitstreamSkip(&bs, 4);

                if (MPEG4_BitstreamGetBit(&bs))
                {
                    MPEG4_BitstreamSkip(&bs, 24);
                }
            }
        }
        else if ((start_code & ~0x0000001F) == 0x00000100)
        {
            MPEG4_BitstreamSkip(&bs, 32);
        }
        else if ((start_code & ~0x0000000F) == 0x00000120)
        {
            MPEG4_BitstreamSkip(&bs, 33);

            if (MPEG4_BitstreamShowBits(&bs, 8) != 1 &&
                    MPEG4_BitstreamShowBits(&bs, 8) != 3 &&
                    MPEG4_BitstreamShowBits(&bs, 8) != 4 &&
                    MPEG4_BitstreamShowBits(&bs, 8) != 0)
            {
                return -1;
            }

            MPEG4_BitstreamSkip(&bs, 8);

            if (MPEG4_BitstreamGetBit(&bs))
            {
                vol_ver_id = MPEG4_BitstreamGetBits(&bs, 4);
                MPEG4_BitstreamSkip(&bs, 3);
            }
            else
            {
                vol_ver_id = 1;
            }

            if (MPEG4_BitstreamGetBits(&bs, 4) == 15)
            {
                MPEG4_BitstreamSkip(&bs, 16);
            }

            if (MPEG4_BitstreamGetBit(&bs))
            {
                MPEG4_BitstreamSkip(&bs, 3);

                if (MPEG4_BitstreamGetBit(&bs))
                {
                    MPEG4_BitstreamSkip(&bs, 81);
                }
            }

            shape = MPEG4_BitstreamGetBits(&bs, 2);
            if ( shape == 3 && vol_ver_id != 1)
            {
                MPEG4_BitstreamSkip(&bs, 4);
            }

            MPEG4_BitstreamSkip(&bs, 1);

            time_increment_resolution = MPEG4_BitstreamGetBits(&bs, 16);

            *framerate = time_increment_resolution;
            if (time_increment_resolution > 0)
            {
                time_inc_bits = MPEG4_log2bin(time_increment_resolution-1);
            }
            else
            {
                time_inc_bits = 1;
            }

            MPEG4_BitstreamSkip(&bs, 1);

            if (MPEG4_BitstreamGetBit(&bs))
            {
                MPEG4_BitstreamSkip(&bs, time_inc_bits);
            }

            if (shape != 2)
            {
                if (shape == 0)
                {
                    MPEG4_BitstreamSkip(&bs, 1);
                    *nWidth = MPEG4_BitstreamGetBits(&bs, 13);
                    MPEG4_BitstreamSkip(&bs, 1);
                    *nHeight = MPEG4_BitstreamGetBits(&bs, 13);
                    return 0;
                }
                return -1;
            }
            else
            {
                if (vol_ver_id != 1)
                {
                    if (MPEG4_BitstreamGetBit(&bs))
                    {
                        return -1;
                    }
                }
                MPEG4_BitstreamSkip(&bs, 1);
            }
        }
        else if (start_code == 0x01b3)
        {
            MPEG4_BitstreamSkip(&bs, 32);
            {
                MPEG4_BitstreamSkip(&bs, 18);
            }
            MPEG4_BitstreamSkip(&bs, 2);
        }
        else if (start_code == 0x01b6)
        {
            MPEG4_BitstreamSkip(&bs, 32);
            coding_type = MPEG4_BitstreamGetBits(&bs, 2);
            frame_type	= coding_type;
            if(coding_type==3)
                coding_type=1;

            while (MPEG4_BitstreamGetBit(&bs) != 0)
            {
                time_incr++;
            }

            MPEG4_BitstreamSkip(&bs, 1);

            if (time_inc_bits)
            {
                time_increment = (MPEG4_BitstreamGetBits(&bs, time_inc_bits));
            }

            MPEG4_BitstreamSkip(&bs, 1);

            if (!MPEG4_BitstreamGetBit(&bs))
            {
                return -1;
            }

            if ((shape != 2) && (coding_type == 1))
            {
                MPEG4_BitstreamSkip(&bs,1);
            }

            if (shape != 0)
            {
                unsigned int width, height;
                unsigned int horiz_mc_ref, vert_mc_ref;

                width = MPEG4_BitstreamGetBits(&bs, 13);
                MPEG4_BitstreamSkip(&bs,1);
                height = MPEG4_BitstreamGetBits(&bs, 13);
                MPEG4_BitstreamSkip(&bs,1);
                horiz_mc_ref = MPEG4_BitstreamGetBits(&bs, 13);
                MPEG4_BitstreamSkip(&bs,1);
                vert_mc_ref = MPEG4_BitstreamGetBits(&bs, 13);
                MPEG4_BitstreamSkip(&bs, 2);

                if (MPEG4_BitstreamGetBit(&bs))
                {
                    MPEG4_BitstreamSkip(&bs, 8);
                }
            }
            return -1;
        }
        else if (start_code == 0x01b2)
        {
            MPEG4_BitstreamSkip(&bs, 48);
        }
        else
        {
            MPEG4_BitstreamSkip(&bs, 32);
        }

    }
    while ((MPEG4_BitstreamPos(&bs) >> 3) < bs.length);

    return -1;
}

/*************************************************************************/
/*                                                                       */
/*                         H264 Analyse function                         */
/*								winton			                         */
/*							  2008-1-31                                  */
/*                                                                       */
/*************************************************************************/


#define uint8_t unsigned char
#define uint32_t unsigned int
#define inline __inline
#define avctx(a)

typedef struct AVRational
{
    int num; ///< numerator
    int den; ///< denominator
} AVRational;

static const AVRational pixel_aspect[17]=
{
    {0, 1},
    {1, 1},
    {12, 11},
    {10, 11},
    {16, 11},
    {40, 33},
    {24, 11},
    {20, 11},
    {32, 11},
    {80, 33},
    {18, 11},
    {15, 11},
    {64, 33},
    {160,99},
    {4, 3},
    {3, 2},
    {2, 1},
};

const uint8_t ff_h264_chroma_qp[52]=
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,
    12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,
    28,29,29,30,31,32,32,33,34,34,35,35,36,36,37,37,
    37,38,38,38,39,39,39,39
};

static const uint8_t default_scaling4[2][16]=
{
    {
        6,13,20,28,
        13,20,28,32,
        20,28,32,37,
        28,32,37,42
    },{
        10,14,20,24,
        14,20,24,27,
        20,24,27,30,
        24,27,30,34
    }
};

static const uint8_t default_scaling8[2][64]=
{
    {
        6,10,13,16,18,23,25,27,
        10,11,16,18,23,25,27,29,
        13,16,18,23,25,27,29,31,
        16,18,23,25,27,29,31,33,
        18,23,25,27,29,31,33,36,
        23,25,27,29,31,33,36,38,
        25,27,29,31,33,36,38,40,
        27,29,31,33,36,38,40,42
    },{
        9,13,15,17,19,21,22,24,
        13,13,17,19,21,22,24,25,
        15,17,19,21,22,24,25,27,
        17,19,21,22,24,25,27,28,
        19,21,22,24,25,27,28,30,
        21,22,24,25,27,28,30,32,
        22,24,25,27,28,30,32,33,
        24,25,27,28,30,32,33,35
    }
};

typedef struct SPS
{
    int profile_idc;
    int level_idc;
    int chroma_format_idc;
    int transform_bypass;              ///< qpprime_y_zero_transform_bypass_flag
    int log2_max_frame_num;            ///< log2_max_frame_num_minus4 + 4
    int poc_type;                      ///< pic_order_cnt_type
    int log2_max_poc_lsb;              ///< log2_max_pic_order_cnt_lsb_minus4
    int delta_pic_order_always_zero_flag;
    int offset_for_non_ref_pic;
    int offset_for_top_to_bottom_field;
    int poc_cycle_length;              ///< num_ref_frames_in_pic_order_cnt_cycle
    int ref_frame_count;               ///< num_ref_frames
    int gaps_in_frame_num_allowed_flag;
    int mb_width;                      ///< pic_width_in_mbs_minus1 + 1
    int mb_height;                     ///< pic_height_in_map_units_minus1 + 1
    int frame_mbs_only_flag;
    int mb_aff;                        ///<mb_adaptive_frame_field_flag
    int direct_8x8_inference_flag;
    int crop;                   ///< frame_cropping_flag
    unsigned int crop_left;            ///< frame_cropping_rect_left_offset
    unsigned int crop_right;           ///< frame_cropping_rect_right_offset
    unsigned int crop_top;             ///< frame_cropping_rect_top_offset
    unsigned int crop_bottom;          ///< frame_cropping_rect_bottom_offset
    int vui_parameters_present_flag;
    AVRational sar;
    int video_signal_type_present_flag;
    int full_range;
    int colour_description_present_flag;
    int timing_info_present_flag;
    uint32_t num_units_in_tick;
    uint32_t time_scale;
    int fixed_frame_rate_flag;
    int bitstream_restriction_flag;
    int num_reorder_frames;
    int scaling_matrix_present;
    uint8_t scaling_matrix4[6][16];
    uint8_t scaling_matrix8[2][64];
    int nal_hrd_parameters_present_flag;
    int vcl_hrd_parameters_present_flag;
    int pic_struct_present_flag;
    int time_offset_length;
    int cpb_cnt;                       ///< See H.264 E.1.2
    int initial_cpb_removal_delay_length; ///< initial_cpb_removal_delay_length_minus1 +1
    int cpb_removal_delay_length;      ///< cpb_removal_delay_length_minus1 + 1
    int dpb_output_delay_length;       ///< dpb_output_delay_length_minus1 + 1
    int bit_depth_luma;                ///< bit_depth_luma_minus8 + 8
    int bit_depth_chroma;              ///< bit_depth_chroma_minus8 + 8
    int residual_color_transform_flag; ///< residual_colour_transform_flag
} SPS;
typedef struct MpegEncContext
{
    GetBitContext gb;
} MpegEncContext;
typedef struct H264Context
{
    MpegEncContext s;
    SPS sps; ///< current sps
} H264Context;
#define get_bits H264_get_bits
#define get_bits1 H264_get_one_bit
#define get_se_golomb H264_get_se_golomb
#define get_ue_golomb H264_get_ue_golomb

static inline int decode_hrd_parameters(H264Context *h, SPS *sps)
{
    //..........
    return 0;
}

static inline int decode_vui_parameters(H264Context *h, SPS *sps)
{
//...
    return 0;
}
static const uint8_t zigzag_scan[16]=
{
    0+0*4, 1+0*4, 0+1*4, 0+2*4,
    1+1*4, 2+0*4, 3+0*4, 2+1*4,
    1+2*4, 0+3*4, 1+3*4, 2+2*4,
    3+1*4, 3+2*4, 2+3*4, 3+3*4,
};
static const uint8_t ff_zigzag_direct[64] =
{
    0,   1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

static void decode_scaling_list(H264Context *h, uint8_t *factors, int size,
                                const uint8_t *jvt_list, const uint8_t *fallback_list)
{
    MpegEncContext * const s = &h->s;
    int i, last = 8, next = 8;
//    const uint8_t *scan = size == 16 ? zigzag_scan : ff_zigzag_direct;
    if(!get_bits1(&s->gb))
    {
        /* matrix not written, we use the predicted one */
        //  memcpy(factors, fallback_list, size*sizeof(uint8_t));
    }
    else
        for(i=0; i<size; i++)
        {
            if(next)
            {
                next = (last + get_se_golomb(&s->gb)) & 0xff;
            }
            if(!i && !next)  /* matrix not written, we use the preset one */
            {
                //   memcpy(factors, jvt_list, size*sizeof(uint8_t));
                break;
            }
            last =  next ? next : last;
        }
}

static void decode_scaling_matrices(H264Context *h, SPS *sps,int is_sps,
                                    uint8_t (*scaling_matrix4)[16], uint8_t (*scaling_matrix8)[64])
{
    MpegEncContext * const s = &h->s;
    int fallback_sps = !is_sps && sps->scaling_matrix_present;
    const uint8_t *fallback[4] =
    {
        fallback_sps ? sps->scaling_matrix4[0] : default_scaling4[0],
        fallback_sps ? sps->scaling_matrix4[3] : default_scaling4[1],
        fallback_sps ? sps->scaling_matrix8[0] : default_scaling8[0],
        fallback_sps ? sps->scaling_matrix8[1] : default_scaling8[1]
    };
    if(get_bits1(&s->gb))
    {
        sps->scaling_matrix_present |= is_sps;
        decode_scaling_list(h,scaling_matrix4[0],16,default_scaling4[0],fallback[0]); // Intra, Y
        decode_scaling_list(h,scaling_matrix4[1],16,default_scaling4[0],scaling_matrix4[0]); // Intra, Cr
        decode_scaling_list(h,scaling_matrix4[2],16,default_scaling4[0],scaling_matrix4[1]); // Intra, Cb
        decode_scaling_list(h,scaling_matrix4[3],16,default_scaling4[1],fallback[1]); // Inter, Y
        decode_scaling_list(h,scaling_matrix4[4],16,default_scaling4[1],scaling_matrix4[3]); // Inter, Cr
        decode_scaling_list(h,scaling_matrix4[5],16,default_scaling4[1],scaling_matrix4[4]); // Inter, Cb
        if(is_sps )
        {
            decode_scaling_list(h,scaling_matrix8[0],64,default_scaling8[0],fallback[2]);  // Intra, Y
            decode_scaling_list(h,scaling_matrix8[1],64,default_scaling8[1],fallback[3]);  // Inter, Y
        }
    }
}

int H264_Analyse(unsigned char* pBuf,int nSize,int* nWidth,int* nHeight,int* framerate)
{
    H264Context h264data;
    H264Context *h=&h264data;
    MpegEncContext * const s = &h->s;
    int profile_idc, level_idc;
    unsigned int sps_id;
    SPS *sps=&h->sps;

    H264_init_get_bits(&s->gb, pBuf+4, nSize-4);

    profile_idc= get_bits(&s->gb, 8);
    get_bits1(&s->gb);   //constraint_set0_flag
    get_bits1(&s->gb);   //constraint_set1_flag
    get_bits1(&s->gb);   //constraint_set2_flag
    get_bits1(&s->gb);   //constraint_set3_flag
    get_bits(&s->gb, 4); // reserved
    level_idc= get_bits(&s->gb, 8);


    sps_id= get_ue_golomb(&s->gb);


    sps->time_offset_length = 24;
    sps->profile_idc= profile_idc;
    sps->level_idc= level_idc;


    sps->scaling_matrix_present = 0;

    if(sps->profile_idc >= 100)  //high profile
    {
        sps->chroma_format_idc= get_ue_golomb(&s->gb);
        if(sps->chroma_format_idc == 3)
            sps->residual_color_transform_flag = get_bits1(&s->gb);
        sps->bit_depth_luma   = get_ue_golomb(&s->gb) + 8;
        sps->bit_depth_chroma = get_ue_golomb(&s->gb) + 8;
        sps->transform_bypass = get_bits1(&s->gb);
        decode_scaling_matrices(h, sps, 1, sps->scaling_matrix4, sps->scaling_matrix8);
    }
    else
    {
        sps->chroma_format_idc= 1;
        sps->bit_depth_luma   = 8;
        sps->bit_depth_chroma = 8;
    }

    sps->log2_max_frame_num= get_ue_golomb(&s->gb) + 4;
    sps->poc_type= get_ue_golomb(&s->gb);

    if(sps->poc_type == 0)  //FIXME #define
    {
        sps->log2_max_poc_lsb= get_ue_golomb(&s->gb) + 4;
    }
    else if(sps->poc_type == 1)
    {
        //FIXME #define
        int i;
        sps->delta_pic_order_always_zero_flag= get_bits1(&s->gb);
        sps->offset_for_non_ref_pic= get_se_golomb(&s->gb);
        sps->offset_for_top_to_bottom_field= get_se_golomb(&s->gb);
        sps->poc_cycle_length                = get_ue_golomb(&s->gb);
        for(i=0; i<sps->poc_cycle_length; i++)
            get_se_golomb(&s->gb);
    }
    else if(sps->poc_type != 2)
    {
        return -1;
    }

    sps->ref_frame_count= get_ue_golomb(&s->gb);

    sps->gaps_in_frame_num_allowed_flag= get_bits1(&s->gb);
    sps->mb_width = get_ue_golomb(&s->gb) + 1;
    sps->mb_height= get_ue_golomb(&s->gb) + 1;


    sps->frame_mbs_only_flag= get_bits1(&s->gb);
    if(!sps->frame_mbs_only_flag)
        sps->mb_aff= get_bits1(&s->gb);
    else
        sps->mb_aff= 0;

    sps->direct_8x8_inference_flag= get_bits1(&s->gb);

    sps->crop= get_bits1(&s->gb);
    if(sps->crop)
    {
        sps->crop_left  = get_ue_golomb(&s->gb);
        sps->crop_right = get_ue_golomb(&s->gb);
        sps->crop_top   = get_ue_golomb(&s->gb);
        sps->crop_bottom= get_ue_golomb(&s->gb);
    }
    else
    {
        sps->crop_left  =
            sps->crop_right =
                sps->crop_top   =
                    sps->crop_bottom= 0;
    }
    *nWidth= 16 * sps->mb_width- 2 * (sps->crop_left + sps->crop_right );

    if (sps->frame_mbs_only_flag)
    {
        *nHeight  = 16 * sps->mb_height- 2 * (sps->crop_top + sps->crop_bottom);
    }
    else
    {
        *nHeight  = 16 * sps->mb_height - 4 * (sps->crop_top +sps->crop_bottom);
    }
    sps->vui_parameters_present_flag= get_bits1(&s->gb);

    return 1;


}




/********************************************************************
	Function Name   :	Stream_Analyse
	Input Param     :	unsigned char* pBuf	：buf地址
						int nSize	：大小
	Output Param    :	int* nWidth	：宽
						int* nHeight：高
	Return          :	-1:failed	0:mpeg4		1:h264
	Description     :	码流分析函数
	Modified Date   :   2008-1-31   09:04
	Modified By     :   Winton
*********************************************************************/
int Stream_Analyse(unsigned char* pBuf,int nSize,int* nWidth,int* nHeight,int* framerate)
{
    int i = 0;
    int ret ;

    if ((!pBuf)||(!nWidth)||(!nHeight))
    {
        return -1;
    }

    while (i < (nSize-4))
    {
        //mpeg4
        if((*(pBuf+i) == 0x00)&&(*(pBuf+i+1) == 0x00)&&(*(pBuf+i+2) == 0x01)&&(*(pBuf+i+3) == 0xB0))
        {
            ret = Mpeg4_Analyse(pBuf+i, nSize-i, nWidth,nHeight,framerate);
            if(ret >= 0)
            {
                return ret;
            }
        }

        //ADI h264
        if( (*(pBuf+i) == 0x00)&&(*(pBuf+i+1) == 0x00)&&(*(pBuf+i+2) == 0x01)
          )
        {
            unsigned char naltype = *(pBuf+i+3);
            //!取低5位才是nal type , rfc3984规定
            naltype = naltype & 0x1f;
            if ( naltype == 7 ) //!sps 内才能分析出信息
            {
                ret = H264_Analyse(pBuf+i, nSize-i, nWidth,nHeight,framerate);
                if(ret >= 0)
                {
                    return ret;
                }
            }
        }

        i++;
    }

    return -1;
}

int decsps(unsigned char* pBuf, unsigned int nSize, int* width, int* height)
{
    int rate;
    int iret = Stream_Analyse(pBuf, nSize,(int*)width,(int*)height,&rate);

    return iret>0 ? 0 : -1;
}




#if 0
int main(int argc, char* argv[])
{
    unsigned char* pBuf;
    FILE* f;
    int len = 0;
    int pos = 0;
    int ret = 0;
    int frame_width = 0;
    int frame_height = 0;
    int readLen = 0x800;
    int framerate = 0;

    pBuf = (unsigned char*)malloc(200*1024);
    if(!pBuf)
    {
        printf("malloc failed\r\n");
        return 0;
    }
    else
    {
        printf("ptotal buf 0x%x \n",pBuf);
    }

    f = fopen("E:\\share\\code\\x264encqp26_0.264","rb");
    if(!f)
    {
        free(pBuf);
        printf("file open failed\r\n");
        return 0;
    }
REPEAT:
    len = fread(pBuf+pos,1,readLen,f);

    if (!len)
    {
        fclose(f);
        free(pBuf);
        printf("file empty\r\n");
        return 0;
    }
    pos += len;



    ret = Stream_Analyse(pBuf,len,&frame_width,&frame_height,&framerate);
    switch(ret)
    {
    case -1:
        goto REPEAT;
        break;
    case 0:
        printf("this file is mpeg4,width=%d,height=%d,framerate=%d\r\n",frame_width,frame_height,framerate);
        break;
    case 1:
        printf("this file is h264,width=%d,height=%d,framerate=%d\r\n",frame_width,frame_height,framerate);
        break;
    }
    fclose(f);
    free(pBuf);

    return 0;
}

#endif





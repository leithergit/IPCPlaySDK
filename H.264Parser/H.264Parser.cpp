// H.264Parser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <Shlwapi.h>
#include <process.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include <conio.h>
#include <math.h>
#pragma comment(lib,"shlwapi.lib")
void PrintUsage(int argc, _TCHAR* argv[])
{
	printf("Usage:\r\n");
	printf("%s File.h264\r\n");
}

byte *pFileBuffer = new byte[256 * 1024];
int	  nBufferSize = 256 * 1024;
int	  nDataLength = 0;
int KMP_StrFind(BYTE *szSource, int nSourceLength, BYTE *szKey, int nKeyLength);
bool h264_decode_sps(BYTE * buf, unsigned int nLen, int &width, int &height, int &fps);

/*
* h264_sps_parser.h
*
* Copyright (c) 2014 Zhou Quan <zhouqicy@gmail.com>
*
* This file is part of ijkPlayer.
*
* ijkPlayer is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* ijkPlayer is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with ijkPlayer; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <inttypes.h>
#include <stdbool.h>
static const uint8_t default_scaling4[2][16] = {
	{ 6, 13, 20, 28, 13, 20, 28, 32,
	20, 28, 32, 37, 28, 32, 37, 42 },
	{ 10, 14, 20, 24, 14, 20, 24, 27,
	20, 24, 27, 30, 24, 27, 30, 34 }
};
static const uint8_t default_scaling8[2][64] = {
	{ 6, 10, 13, 16, 18, 23, 25, 27,
	10, 11, 16, 18, 23, 25, 27, 29,
	13, 16, 18, 23, 25, 27, 29, 31,
	16, 18, 23, 25, 27, 29, 31, 33,
	18, 23, 25, 27, 29, 31, 33, 36,
	23, 25, 27, 29, 31, 33, 36, 38,
	25, 27, 29, 31, 33, 36, 38, 40,
	27, 29, 31, 33, 36, 38, 40, 42 },
	{ 9, 13, 15, 17, 19, 21, 22, 24,
	13, 13, 17, 19, 21, 22, 24, 25,
	15, 17, 19, 21, 22, 24, 25, 27,
	17, 19, 21, 22, 24, 25, 27, 28,
	19, 21, 22, 24, 25, 27, 28, 30,
	21, 22, 24, 25, 27, 28, 30, 32,
	22, 24, 25, 27, 28, 30, 32, 33,
	24, 25, 27, 28, 30, 32, 33, 35 }
};
static const uint8_t ff_zigzag_direct[64] = {
	0, 1, 8, 16, 9, 2, 3, 10,
	17, 24, 32, 25, 18, 11, 4, 5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13, 6, 7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63
};
static const uint8_t ff_zigzag_scan[16 + 1] = {
	0 + 0 * 4, 1 + 0 * 4, 0 + 1 * 4, 0 + 2 * 4,
	1 + 1 * 4, 2 + 0 * 4, 3 + 0 * 4, 2 + 1 * 4,
	1 + 2 * 4, 0 + 3 * 4, 1 + 3 * 4, 2 + 2 * 4,
	3 + 1 * 4, 3 + 2 * 4, 2 + 3 * 4, 3 + 3 * 4,
};
struct nal_bitstream
{
	const uint8_t *data;
	const uint8_t *end;
	int head;
	uint64_t cache;
} ;
static void nal_bs_init(nal_bitstream *bs, const uint8_t *data, size_t size)
{
	bs->data = data;
	bs->end = data + size;
	bs->head = 0;
	// fill with something other than 0 to detect
	//  emulation prevention bytes
	bs->cache = 0xffffffff;
}
static uint64_t nal_bs_read(nal_bitstream *bs, int n)
{
	uint64_t res = 0;
	int shift;

	if (n == 0)
		return res;

	// fill up the cache if we need to
	while (bs->head < n) {
		uint8_t a_byte;
		bool check_three_byte;

		check_three_byte = true;
	next_byte:
		if (bs->data >= bs->end) {
			// we're at the end, can't produce more than head number of bits
			n = bs->head;
			break;
		}
		// get the byte, this can be an emulation_prevention_three_byte that we need
		// to ignore.
		a_byte = *bs->data++;
		if (check_three_byte && a_byte == 0x03 && ((bs->cache & 0xffff) == 0)) {
			// next byte goes unconditionally to the cache, even if it's 0x03
			check_three_byte = false;
			goto next_byte;
		}
		// shift bytes in cache, moving the head bits of the cache left
		bs->cache = (bs->cache << 8) | a_byte;
		bs->head += 8;
	}

	// bring the required bits down and truncate
	if ((shift = bs->head - n) > 0)
		res = bs->cache >> shift;
	else
		res = bs->cache;

	// mask out required bits
	if (n < 32)
		res &= (1 << n) - 1;

	bs->head = shift;

	return res;
}
static bool nal_bs_eos(nal_bitstream *bs)
{
	return (bs->data >= bs->end) && (bs->head == 0);
}
// read unsigned Exp-Golomb code
static int64_t nal_bs_read_ue(nal_bitstream *bs)
{
	int i = 0;

	while (nal_bs_read(bs, 1) == 0 && !nal_bs_eos(bs) && i < 32)
		i++;

	return ((1 << i) - 1 + nal_bs_read(bs, i));
}
//解码有符号的指数哥伦布编码
static int64_t nal_bs_read_se(nal_bitstream *bs)
{
	int64_t ueVal = nal_bs_read_ue(bs);
	double k = ueVal;
	int64_t nValue = ceil(k / 2);
	if (ueVal % 2 == 0)
	{
		nValue = -nValue;
	}
	return nValue;
}
struct sps_info_struct
{
	uint64_t profile_idc;
	uint64_t level_idc;
	uint64_t sps_id;

	uint64_t chroma_format_idc;
	uint64_t separate_colour_plane_flag;
	uint64_t bit_depth_luma_minus8;
	uint64_t bit_depth_chroma_minus8;
	uint64_t qpprime_y_zero_transform_bypass_flag;
	uint64_t seq_scaling_matrix_present_flag;

	uint64_t log2_max_frame_num_minus4;
	uint64_t pic_order_cnt_type;
	uint64_t log2_max_pic_order_cnt_lsb_minus4;

	uint64_t max_num_ref_frames;
	uint64_t gaps_in_frame_num_value_allowed_flag;
	uint64_t pic_width_in_mbs_minus1;
	uint64_t pic_height_in_map_units_minus1;

	uint64_t frame_mbs_only_flag;
	uint64_t mb_adaptive_frame_field_flag;

	uint64_t direct_8x8_inference_flag;

	uint64_t frame_cropping_flag;
	uint64_t frame_crop_left_offset;
	uint64_t frame_crop_right_offset;
	uint64_t frame_crop_top_offset;
	uint64_t frame_crop_bottom_offset;
} ;
static void decode_scaling_list(nal_bitstream *bs, uint8_t *factors, int size,
	const uint8_t *jvt_list,
	const uint8_t *fallback_list)
{
	int i, last = 8, next = 8;
	const uint8_t *scan = size == 16 ? ff_zigzag_scan : ff_zigzag_direct;
	if (!nal_bs_read(bs, 1)) /* matrix not written, we use the predicted one */
		memcpy(factors, fallback_list, size * sizeof(uint8_t));
	else
		for (i = 0; i < size; i++) {
			if (next)
				next = (last + nal_bs_read_se(bs)) & 0xff;
			if (!i && !next) { /* matrix not written, we use the preset one */
				memcpy(factors, jvt_list, size * sizeof(uint8_t));
				break;
			}
			last = factors[scan[i]] = next ? next : last;
		}
}
static void decode_scaling_matrices(nal_bitstream *bs, sps_info_struct *sps,
	void* pps, int is_sps,
	uint8_t(*scaling_matrix4)[16],
	uint8_t(*scaling_matrix8)[64])
{
	int fallback_sps = !is_sps && sps->seq_scaling_matrix_present_flag;
	const uint8_t *fallback[4] = {
		fallback_sps ? scaling_matrix4[0] : default_scaling4[0],
		fallback_sps ? scaling_matrix4[3] : default_scaling4[1],
		fallback_sps ? scaling_matrix8[0] : default_scaling8[0],
		fallback_sps ? scaling_matrix8[3] : default_scaling8[1]
	};
	if (nal_bs_read(bs, 1)) {
		sps->seq_scaling_matrix_present_flag |= is_sps;
		decode_scaling_list(bs, scaling_matrix4[0], 16, default_scaling4[0], fallback[0]);        // Intra, Y
		decode_scaling_list(bs, scaling_matrix4[1], 16, default_scaling4[0], scaling_matrix4[0]); // Intra, Cr
		decode_scaling_list(bs, scaling_matrix4[2], 16, default_scaling4[0], scaling_matrix4[1]); // Intra, Cb
		decode_scaling_list(bs, scaling_matrix4[3], 16, default_scaling4[1], fallback[1]);        // Inter, Y
		decode_scaling_list(bs, scaling_matrix4[4], 16, default_scaling4[1], scaling_matrix4[3]); // Inter, Cr
		decode_scaling_list(bs, scaling_matrix4[5], 16, default_scaling4[1], scaling_matrix4[4]); // Inter, Cb
		if (is_sps) {
			decode_scaling_list(bs, scaling_matrix8[0], 64, default_scaling8[0], fallback[2]); // Intra, Y
			decode_scaling_list(bs, scaling_matrix8[3], 64, default_scaling8[1], fallback[3]); // Inter, Y
			if (sps->chroma_format_idc == 3) {
				decode_scaling_list(bs, scaling_matrix8[1], 64, default_scaling8[0], scaling_matrix8[0]); // Intra, Cr
				decode_scaling_list(bs, scaling_matrix8[4], 64, default_scaling8[1], scaling_matrix8[3]); // Inter, Cr
				decode_scaling_list(bs, scaling_matrix8[2], 64, default_scaling8[0], scaling_matrix8[1]); // Intra, Cb
				decode_scaling_list(bs, scaling_matrix8[5], 64, default_scaling8[1], scaling_matrix8[4]); // Inter, Cb
			}
		}
	}
}
static void Parserh264_sps(sps_info_struct* sps_info, uint8_t* sps_data, uint32_t sps_size)
{
	if (sps_info == NULL)
		return;
	memset(sps_info, 0, sizeof(sps_info_struct));

	uint8_t scaling_matrix4[6][16];
	uint8_t scaling_matrix8[6][64];
	memset(scaling_matrix4, 16, sizeof(scaling_matrix4));
	memset(scaling_matrix8, 16, sizeof(scaling_matrix8));

	nal_bitstream bs;
	nal_bs_init(&bs, sps_data, sps_size);

	sps_info->profile_idc = nal_bs_read(&bs, 8);
	nal_bs_read(&bs, 1);  // constraint_set0_flag
	nal_bs_read(&bs, 1);  // constraint_set1_flag
	nal_bs_read(&bs, 1);  // constraint_set2_flag
	nal_bs_read(&bs, 1);  // constraint_set3_flag
	nal_bs_read(&bs, 4);  // reserved
	sps_info->level_idc = nal_bs_read(&bs, 8);
	sps_info->sps_id = nal_bs_read_ue(&bs);

	if (sps_info->profile_idc == 100 ||
		sps_info->profile_idc == 110 ||
		sps_info->profile_idc == 122 ||
		sps_info->profile_idc == 244 ||
		sps_info->profile_idc == 44 ||
		sps_info->profile_idc == 83 ||
		sps_info->profile_idc == 86 ||
		sps_info->profile_idc == 118 ||
		sps_info->profile_idc == 128 ||
		sps_info->profile_idc == 138 ||
		sps_info->profile_idc == 144)
	{
		sps_info->chroma_format_idc = nal_bs_read_ue(&bs);
		if (sps_info->chroma_format_idc == 3)
			sps_info->separate_colour_plane_flag = nal_bs_read(&bs, 1);
		sps_info->bit_depth_luma_minus8 = nal_bs_read_ue(&bs);
		sps_info->bit_depth_chroma_minus8 = nal_bs_read_ue(&bs);
		sps_info->qpprime_y_zero_transform_bypass_flag = nal_bs_read(&bs, 1);

		//        sps_info->seq_scaling_matrix_present_flag = nal_bs_read (&bs, 1);
		decode_scaling_matrices(&bs, sps_info, NULL, 1, scaling_matrix4, scaling_matrix8);

	}
	sps_info->log2_max_frame_num_minus4 = nal_bs_read_ue(&bs);
	if (sps_info->log2_max_frame_num_minus4 > 12) {
		// must be between 0 and 12
		// don't early return here - the bits we are using (profile/level/interlaced/ref frames)
		// might still be valid - let the parser go on and pray.
		//return;
	}

	sps_info->pic_order_cnt_type = nal_bs_read_ue(&bs);
	if (sps_info->pic_order_cnt_type == 0) {
		sps_info->log2_max_pic_order_cnt_lsb_minus4 = nal_bs_read_ue(&bs);
	}
	else if (sps_info->pic_order_cnt_type == 1) {
		nal_bs_read(&bs, 1); //delta_pic_order_always_zero_flag
		nal_bs_read_se(&bs); //offset_for_non_ref_pic
		nal_bs_read_se(&bs); //offset_for_top_to_bottom_field

		int64_t num_ref_frames_in_pic_order_cnt_cycle = nal_bs_read_ue(&bs);
		for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
		{
			nal_bs_read_se(&bs); //int offset_for_ref_frame =
		}
	}

	sps_info->max_num_ref_frames = nal_bs_read_ue(&bs);
	sps_info->gaps_in_frame_num_value_allowed_flag = nal_bs_read(&bs, 1);
	sps_info->pic_width_in_mbs_minus1 = nal_bs_read_ue(&bs);
	sps_info->pic_height_in_map_units_minus1 = nal_bs_read_ue(&bs);

	sps_info->frame_mbs_only_flag = nal_bs_read(&bs, 1);
	if (!sps_info->frame_mbs_only_flag)
		sps_info->mb_adaptive_frame_field_flag = nal_bs_read(&bs, 1);

	sps_info->direct_8x8_inference_flag = nal_bs_read(&bs, 1);

	sps_info->frame_cropping_flag = nal_bs_read(&bs, 1);
	if (sps_info->frame_cropping_flag) {
		sps_info->frame_crop_left_offset = nal_bs_read_ue(&bs);
		sps_info->frame_crop_right_offset = nal_bs_read_ue(&bs);
		sps_info->frame_crop_top_offset = nal_bs_read_ue(&bs);
		sps_info->frame_crop_bottom_offset = nal_bs_read_ue(&bs);
	}
}
static void get_resolution_from_sps(uint8_t* sps, uint32_t sps_size, uint32_t* width, uint32_t* height/*,uint32_t *fps*/)
{
	sps_info_struct sps_info = { 0 };

	Parserh264_sps(&sps_info, sps, sps_size);

	// 宽高计算公式
	*width = (sps_info.pic_width_in_mbs_minus1 + 1) * 16;
	*height = (2 - sps_info.frame_mbs_only_flag)* (sps_info.pic_height_in_map_units_minus1 + 1) * 16;

	if (sps_info.frame_cropping_flag)
	{
		unsigned int crop_unit_x;
		unsigned int crop_unit_y;
		if (0 == sps_info.chroma_format_idc) // monochrome
		{
			crop_unit_x = 1;
			crop_unit_y = 2 - sps_info.frame_mbs_only_flag;
		}
		else if (1 == sps_info.chroma_format_idc) // 4:2:0
		{
			crop_unit_x = 2;
			crop_unit_y = 2 * (2 - sps_info.frame_mbs_only_flag);
		}
		else if (2 == sps_info.chroma_format_idc) // 4:2:2
		{
			crop_unit_x = 2;
			crop_unit_y = 2 - sps_info.frame_mbs_only_flag;
		}
		else // 3 == sps.chroma_format_idc   // 4:4:4
		{
			crop_unit_x = 1;
			crop_unit_y = 2 - sps_info.frame_mbs_only_flag;
		}

		*width -= crop_unit_x * (sps_info.frame_crop_left_offset + sps_info.frame_crop_right_offset);
		*height -= crop_unit_y * (sps_info.frame_crop_top_offset + sps_info.frame_crop_bottom_offset);
	}

// 	*width = (sps_info.pic_width_in_mbs_minus1 + 1) * 16 - sps_info.frame_crop_left_offset * 2 - sps_info.frame_crop_right_offset * 2;
// 	*height = ((2 - sps_info.frame_mbs_only_flag) * (sps_info.pic_height_in_map_units_minus1 + 1) * 16) - sps_info.frame_crop_top_offset * 2 - sps_info.frame_crop_bottom_offset * 2;

}


typedef int(*video_frame_cb)(int stream, char *frame, unsigned long len, int key, double pts);

typedef struct split_frame_s
{
	unsigned char buffer[1 * 1024 * 1024];
	unsigned char dst[1 * 1024 * 1024];
	unsigned char idr[1 * 1024 * 1024];
	int idr_len;
	unsigned char vps[128]; //only h265
	int vps_len;
	unsigned char sps[128];
	int sps_len;
	unsigned char pps[128];
	int pps_len;
	unsigned char sei[128];
	int sei_len;
	FILE* fp;
	int file_type; // 0->264, 1->265
	video_frame_cb frame_cb;
	double pts;

	int running;
	HANDLE hThread;
}split_frame_s;
static split_frame_s g_arg;

int split_frame_264(unsigned char* src, int s_len, int* r_len, unsigned char* dst, int* len)
{
	*r_len = 0;
	*len = 0;
	int i = 0;
	int j = 0;
	int valid = 0;
	for (i = 0; i < s_len; i++)
	{
		if (i + 3 < s_len && src[i] == 0 && src[i + 1] == 0 && src[i + 2] == 0 && src[i + 3] == 1)
		{
			for (j = i + 3; j < s_len; j++)
			{
				if (j + 3 < s_len && src[j] == 0 && src[j + 1] == 0 && src[j + 2] == 0 && src[j + 3] == 1)
				{
					valid = 1;
					break;
				}
			}
			break;
		}
	}

	if (valid)
	{
		memcpy(dst, src + i, j - i);
		*len = j - i;
		*r_len = j;
	}

	return 0;
}

/*
src: input sources
s_len: input sources length
r_len: output handled length
dst: destination
len: destination length
*/
int split_frame_265(unsigned char* src, int s_len, int* r_len, unsigned char* dst, int* len)
{
	*r_len = 0;
	*len = 0;
	int i = 0;
	int j = 0;
	int valid = 0;
	for (i = 0; i < s_len; i++)
	{
		if (i + 5 < s_len && src[i] == 0 && src[i + 1] == 0 && src[i + 2] == 0 && src[i + 3] == 1 && src[i + 5] == 1)
		{
			//printf("%d %02x %02x %02x %02x %02x %02x\n", __LINE__, src[i], src[i+1], src[i+2], src[i+3], src[i+4], src[i+5]);
			for (j = i + 3; j < s_len; j++)
			{
				if (j + 5 < s_len && src[j] == 0 && src[j + 1] == 0 && src[j + 2] == 0 && src[j + 3] == 1 && src[j + 5] == 1)
				{
					//printf("%d %02x %02x %02x %02x %02x %02x\n", __LINE__, src[j], src[j+1], src[j+2], src[j+3], src[j+4], src[j+5]);
					valid = 1;
					break;
				}
			}
			break;
		}
	}

	if (valid)
	{
		memcpy(dst, &src[i], j - i);
		*len = j - i;
		*r_len = j;
	}

	return 0;
}

UINT __stdcall  split_frame_proc(void* arg)
{
	int r_len = 0;
	int d_len = 0;
	int writable = 0;

	while (g_arg.running)
	{
		int n = fread(g_arg.buffer + writable, 1, sizeof(g_arg.buffer) - writable, g_arg.fp);
		if (n > 0)
		{
			writable += n;
		}

		if (writable > 0)
		{
			if (g_arg.file_type)
			{
				split_frame_265(g_arg.buffer, writable, &r_len, g_arg.dst, &d_len);
				if (r_len > 0 && d_len > 0)
				{
					memmove(g_arg.buffer, g_arg.buffer + r_len, sizeof(g_arg.buffer) - r_len);
					writable = sizeof(g_arg.buffer) - r_len;
					//printf("%d %02x %02x %02x %02x %02x %02x, len: %d\n", __LINE__, g_arg.dst[0], g_arg.dst[1], g_arg.dst[2], g_arg.dst[3], g_arg.dst[4], g_arg.dst[5], d_len);

					if (g_arg.dst[4] == 0x40)
					{
						memset(g_arg.vps, 0, sizeof(g_arg.vps));
						memcpy(g_arg.vps, g_arg.dst, d_len);
						g_arg.vps_len = d_len;
					}
					if (g_arg.dst[4] == 0x42)
					{
						memset(g_arg.sps, 0, sizeof(g_arg.sps));
						memcpy(g_arg.sps, g_arg.dst, d_len);
						g_arg.sps_len = d_len;
					}
					if (g_arg.dst[4] == 0x44)
					{
						memset(g_arg.pps, 0, sizeof(g_arg.pps));
						memcpy(g_arg.pps, g_arg.dst, d_len);
						g_arg.pps_len = d_len;
					}
					if (g_arg.dst[4] == 0x4e)
					{
						memset(g_arg.sei, 0, sizeof(g_arg.sei));
						memcpy(g_arg.sei, g_arg.dst, d_len);
						g_arg.sei_len = d_len;
					}
					if (g_arg.dst[4] == 0x26)
					{
						memset(g_arg.idr, 0, sizeof(g_arg.idr));
						memcpy(g_arg.idr, g_arg.dst, d_len);
						g_arg.idr_len = d_len;
					}
					if (g_arg.frame_cb && g_arg.dst[4] == 0x26)
					{
						memset(g_arg.dst, 0, sizeof(g_arg.dst));
						int idx = 0;

						memcpy(g_arg.dst + idx, g_arg.vps, g_arg.vps_len);
						idx += g_arg.vps_len;
						printf("%d len: %d\n", __LINE__, idx);
						memcpy(g_arg.dst + idx, g_arg.sps, g_arg.sps_len);
						idx += g_arg.sps_len;
						printf("%d len: %d\n", __LINE__, idx);
						memcpy(g_arg.dst + idx, g_arg.pps, g_arg.pps_len);
						idx += g_arg.pps_len;
						printf("%d len: %d\n", __LINE__, idx);
						/*
						memcpy(g_arg.dst+idx, g_arg.sei, g_arg.sei_len);
						idx += g_arg.sei_len;
						printf("%d len: %d\n", __LINE__, idx);
						*/
						memcpy(g_arg.dst + idx, g_arg.idr, g_arg.idr_len);
						idx += g_arg.idr_len;
						printf("%d len: %d\n", __LINE__, idx);

						g_arg.frame_cb(0, (char*)g_arg.dst, idx, 1, g_arg.pts);
						//g_arg.frame_cb(1, (char*)g_arg.dst, idx, 1, g_arg.pts);
						g_arg.pts += 40;
					}
					else if (g_arg.frame_cb && g_arg.dst[4] == 0x02)
					{
						g_arg.frame_cb(0, (char*)g_arg.dst, d_len, 0, g_arg.pts);
						//g_arg.frame_cb(1, (char*)g_arg.dst, d_len, 0, g_arg.pts);
						g_arg.pts += 40;
					}
				}
			}
			else
			{
				split_frame_264(g_arg.buffer, writable, &r_len, g_arg.dst, &d_len);
				if (r_len > 0 && d_len > 0)
				{
					memmove(g_arg.buffer, g_arg.buffer + r_len, sizeof(g_arg.buffer) - r_len);
					writable = sizeof(g_arg.buffer) - r_len;
					//printf("%d %02x %02x %02x %02x %02x, len: %d\n", __LINE__, g_arg.dst[0], g_arg.dst[1], g_arg.dst[2], g_arg.dst[3], g_arg.dst[4], d_len);

					if (g_arg.dst[4] == 0x67)
					{
						memset(g_arg.sps, 0, sizeof(g_arg.sps));
						memcpy(g_arg.sps, g_arg.dst, d_len);
						g_arg.sps_len = d_len;
					}
					if (g_arg.dst[4] == 0x68)
					{
						memset(g_arg.pps, 0, sizeof(g_arg.pps));
						memcpy(g_arg.pps, g_arg.dst, d_len);
						g_arg.pps_len = d_len;
					}
					if (g_arg.dst[4] == 0x06)
					{
						memset(g_arg.sei, 0, sizeof(g_arg.sei));
						memcpy(g_arg.sei, g_arg.dst, d_len);
						g_arg.sei_len = d_len;
					}
					if (g_arg.dst[4] == 0x65)
					{
						memset(g_arg.idr, 0, sizeof(g_arg.idr));
						memcpy(g_arg.idr, g_arg.dst, d_len);
						g_arg.idr_len = d_len;
					}
					if (g_arg.frame_cb && g_arg.dst[4] == 0x65)
					{
						memset(g_arg.dst, 0, sizeof(g_arg.dst));
						int idx = 0;

						memcpy(g_arg.dst + idx, g_arg.sps, g_arg.sps_len);
						idx += g_arg.sps_len;
						printf("sps len: %d\n", g_arg.sps_len);
						uint32_t nWidth, nHeight,nfps;
						//h264_decode_sps(g_arg.sps, g_arg.sps_len, nWidth, nHeight, nfps);
						get_resolution_from_sps(g_arg.sps, g_arg.sps_len, &nWidth, &nHeight);
						memcpy(g_arg.dst + idx, g_arg.pps, g_arg.pps_len);
						idx += g_arg.pps_len;
						printf("pps len: %d\n", g_arg.pps_len);
						memcpy(g_arg.dst + idx, g_arg.sei, g_arg.sei_len);
						idx += g_arg.sei_len;
						printf("sei len: %d\n", g_arg.sei_len);
						memcpy(g_arg.dst + idx, g_arg.idr, g_arg.idr_len);
						idx += g_arg.idr_len;
						printf("idr len: %d\n", g_arg.idr_len);

						g_arg.frame_cb(0, (char*)g_arg.dst, idx, 1, g_arg.pts);
						//g_arg.frame_cb(1, (char*)g_arg.dst, idx, 1, g_arg.pts);
						g_arg.pts += 40;
					}
					else if (g_arg.frame_cb && g_arg.dst[4] == 0x61)
					{
						g_arg.frame_cb(0, (char*)g_arg.dst, d_len, 0, g_arg.pts);
						//g_arg.frame_cb(1, (char*)g_arg.dst, d_len, 0, g_arg.pts);
						g_arg.pts += 40;
					}
				}
			}
		}

		if (feof(g_arg.fp))
		{
			fseek(g_arg.fp, 0L, SEEK_SET);
		}

		Sleep(40);
	}

	fclose(g_arg.fp);

	return 0;
}

int split_frame_init(const char* path, video_frame_cb frame_cb)
{
	g_arg.fp = fopen(path, "r");
	assert(g_arg.fp != NULL);

	g_arg.file_type = strstr(path, "265") ? 1 : 0;
	g_arg.frame_cb = frame_cb;

	g_arg.running = 1;
	g_arg.hThread = (HANDLE)_beginthreadex(nullptr,0, split_frame_proc,&g_arg, 0,nullptr);

	return 0;
}

int split_frame_exit()
{
	if (g_arg.running)
	{
		g_arg.running = 0;
		WaitForSingleObject(g_arg.hThread, INFINITE);
	}

	if (g_arg.fp)
	{
		fclose(g_arg.fp);
	}

	return 0;
}

// test
/*
int main(int argc, char** argv)
{
if (argc != 2)
{
printf("Usage: %s [file]\n", argv[0]);
printf("sample: %s hik.265\n", argv[0]);
printf("sample: %s hik.264\n", argv[0]);
return 0;
}

split_frame_init(argv[1]);
getchar();

return 0;
}
*/

int FrameCB(int stream, char *frame, unsigned long len, int key, double pts)
{
	printf("Stream = %d\tLength = %d\tkey = %d\tpts = %.6f\tNALU Type = %d\tFrame = %02x %02x %02x %02x %02x %02x %02x %02x\n",
		stream,
		len,
		key,
		pts,
		(byte)frame[4]&0x1F,
		(byte)frame[0],
		(byte)frame[1],
		(byte)frame[2],
		(byte)frame[3],
		(byte)frame[4],
		(byte)frame[5],
		(byte)frame[6],
		(byte)frame[7]
		);
	return 0;
}
char *szDestFile = nullptr;
bool bThreadRun = false;
HANDLE hThreadRun = nullptr;
#include <time.h>

struct DHFrame
{
	int 	nCodeType;
	byte 	nFrameRate;
	int 	nFrameSeq;
	WORD 	nWidth;
	WORD	nHeight;
	time_t 	tTimeStamp;
	int 	nFrameSize;
	byte 	nExtNum;
	bool	bKeyFrame;
	WORD 	nExtType;
	WORD 	nExtLen;
	byte	nSecond;
	byte 	nMinute;
	byte 	nHour;
	byte 	nDay;
	byte 	nMonth;
	WORD	nYear;
	byte 	*pHeader;
	byte	*pContent;
	DHFrame()
	{
		ZeroMemory(this, sizeof(DHFrame));
	}
	~DHFrame()
	{
		if (pContent)
		{
			delete[]pContent;
			pContent = nullptr;
		}
	}
};

#define _MaxBuffSize	(512*1024)

class CDHStreamParser
{
	byte *pBuffer;
	int		nBufferSize;
	int		nDataLength;
	CRITICAL_SECTION csBuffer;
	int		nSequence;
	time_t	tLastFrameTime;
	int		nFrameInterval;
	DHFrame* pSaveFrame;	// 上次解析未使用的帧
	CDHStreamParser()
	{
	}
public:
	CDHStreamParser(byte *pData, int nDataLen)
	{
		ZeroMemory(this, sizeof(CDHStreamParser));
		::InitializeCriticalSection(&csBuffer);
		nBufferSize = 65535;
		while (nBufferSize < nDataLength)
			nBufferSize *= 2;
		pBuffer = new byte[nBufferSize];
		memcpy(pBuffer, pData, nDataLen);
		nDataLength = nDataLen;
		nFrameInterval = 40;
	}
	~CDHStreamParser()
	{
		::EnterCriticalSection(&csBuffer);
		if (pBuffer)
			delete[] pBuffer;
		if (pSaveFrame)
			delete pSaveFrame;
		nDataLength = 0;
		nBufferSize = 0;
		pBuffer = nullptr;
		::LeaveCriticalSection(&csBuffer);
		::DeleteCriticalSection(&csBuffer);
	}
	// 存入临时帧
	bool SaveFrame(DHFrame *pDHFrame)
	{
		if (pSaveFrame || !pDHFrame)
			return false;
		pSaveFrame = pDHFrame;
		return true;
	}
	// 取出临时帧
	DHFrame *GetSavedFrame()
	{
		if (!pSaveFrame)
			return nullptr;
		DHFrame *pTempFrame = pSaveFrame;
		pSaveFrame = nullptr;
		return pTempFrame;
	}
	int InputStream(byte *pData, int nInputLen)
	{
		if ((nDataLength + nInputLen) < nBufferSize)
		{
			memcpy_s(&pBuffer[nDataLength], nBufferSize - nDataLength, pData, nInputLen);
			nDataLength += nInputLen;
		}
		else
		{
			if (nBufferSize * 2 > _MaxBuffSize)
				return -11;
			//MemMerge((char **)&pBuffer, nDataLength, nBufferSize, (char *)pData, nInputLen);
			return 0;
		}
		return 0;
	}
	DHFrame *ParserFrame()
	{
		int nFrameSize = 0;
		int nExtLen = 0;
		static const byte 	szFlag[3] = { 0x00, 0x00, 0x01 };
		static const byte 	I_FRAME_FLAG = 0xED;
		static const byte 	P_FRAME_C_FLAG = 0xEC;
		static const byte 	P_FRAME_A_FLAG = 0xEA;
		int nOffset = KMP_StrFind(pBuffer, nDataLength, (byte *)szFlag, 3);
		if (nOffset < 0)
			return nullptr;
		byte *pData = pBuffer + nOffset;
		if (pData[3] != I_FRAME_FLAG &&
			pData[3] != P_FRAME_A_FLAG &&
			pData[3] != P_FRAME_C_FLAG)
			return nullptr;


		int extNum = pData[18];
		unsigned char* extptr = pData + 16;
		nExtLen = 0;
		while (extNum > 0)
		{
			int exttype = extptr[0] | extptr[1] << 8;
			int extlen = 4 + extptr[2] | extptr[3] << 8;//2字节类型，2字节长度
			nExtLen += extlen;
			if (nDataLength < 20 + nExtLen)
				return nullptr;
			extptr += extlen;
			extNum--;
		}
		// pFrame->nFrameSize = pData[23] << 24 | pData[22] << 16 | pData[21] << 8 | pData[20];
		nFrameSize = pData[23] << 24 | pData[22] << 16 | pData[21] << 8 | pData[20];
		//pFrame->nFrameSize = pFrame->nFrameSize + 24 + pFrame->nExtLen;
		if (nDataLength < (nFrameSize + 24))
			return nullptr;

		DHFrame *pFrame = new DHFrame;
		if (!pFrame)
			return nullptr;
		pFrame->nFrameSize = nFrameSize;
		pFrame->nExtLen = nExtLen;

#ifndef _DEBUG
		pFrame->pContent = new byte[pFrame->nFrameSize];
#else
		pFrame->pContent = new byte[pFrame->nFrameSize + 24];
#endif
		if (!pFrame->pContent)
		{
			delete pFrame;
			return nullptr;
		}

		pFrame->nCodeType = pData[4];
		pFrame->nFrameRate = pData[6];
		pFrame->nFrameSeq = nSequence++;
		pFrame->nWidth = pData[8] | pData[9] << 8;
		pFrame->nHeight = pData[10] | pData[11] << 8;
		long nDT = pData[12] | pData[13] << 8 | pData[14] << 16 | pData[15] << 24;
		pFrame->nSecond = nDT & 0x3f;
		pFrame->nMinute = (nDT >> 6) & 0x3f;
		pFrame->nHour = (nDT >> 12) & 0x1f;
		pFrame->nDay = (nDT >> 17) & 0x1f;
		pFrame->nMonth = (nDT >> 22) & 0x1f;
		pFrame->nYear = 2000 + (nDT >> 26);

		tm tmp_time;
		tmp_time.tm_hour = pFrame->nHour;
		tmp_time.tm_min = pFrame->nMinute;
		tmp_time.tm_sec = pFrame->nSecond;
		tmp_time.tm_mday = pFrame->nDay;
		tmp_time.tm_mon = pFrame->nMonth - 1;
		tmp_time.tm_year = pFrame->nYear - 1900;
		pFrame->tTimeStamp = mktime(&tmp_time);
		if (pFrame->nFrameRate)
			nFrameInterval = 1000 / pFrame->nFrameRate;
		else
			nFrameInterval = 40;

		// 以I帧为基准，重新规整每一帧的播放时间
		if (pData[3] == I_FRAME_FLAG)
		{
			pFrame->bKeyFrame = true;
			tLastFrameTime = pFrame->tTimeStamp * 1000;
		}
		else
		{
			tLastFrameTime += 40;
		}

		pFrame->tTimeStamp = tLastFrameTime;

#ifdef _DEBUG
		ZeroMemory(pFrame->pContent, pFrame->nFrameSize + 16);
#endif
		memcpy_s(pFrame->pContent, pFrame->nFrameSize + 24, pData, pFrame->nFrameSize + 24);
		memmove(pBuffer, pData + pFrame->nFrameSize, nDataLength - (pFrame->nFrameSize + 24));
		ZeroMemory(&pBuffer[nDataLength], nBufferSize - nDataLength);
		nDataLength -= (pFrame->nFrameSize + 24);
		return pFrame;
	}
};
UINT __stdcall DHFileParserRun(void *)
{
	byte	*pBuffer = new byte[256 * 1024];
	UINT	nBufferSize = 256 * 1024;
	DWORD	nDataLen = 0;
	FILE	*fp = nullptr;
	fp = fopen(szDestFile, "rb");
	if (!fp)
	{
		printf("Failed in Open file %s.", szDestFile);
		return 1;
	}
	int nResult = fread(pBuffer, nBufferSize, 1, fp);
	CDHStreamParser parser(pBuffer, nBufferSize);
	DHFrame *pDHFrame = nullptr;
	while (bThreadRun)
	{
		pDHFrame = parser.ParserFrame();
		while (pDHFrame)
		{
			pDHFrame = parser.ParserFrame();

		}

		int nResult = fread(pBuffer, nBufferSize, 1, fp);
		if (nResult)
			parser.InputStream(pBuffer, nBufferSize);
		else
			break;
	}
	if (pDHFrame)
		delete pDHFrame;
	if (pBuffer)
		delete[]pBuffer;
	if (fp)
		fclose(fp);
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 2)
	{
		PrintUsage(argc, argv);
		return 1;
	}
	if (!PathFileExists(argv[1]))
	{
		printf("Can't find file %s.\r\n", argv[1]);
		return 2;
	}
	szDestFile = argv[1];
	split_frame_init(argv[1], FrameCB);
	system("pause");
	//split_frame_exit();
	
	return 0;
}


//KMP匹配算法模式串的预处理
//返回true,执行成功
//返回false,则为pNext空间不足
bool GetNext(byte *szKey, int nKeyLength, int *pNext, int nNextLength)
{
	int i = 0, j = -1;
	pNext[0] = -1;
	if (nKeyLength <= 0)
		nKeyLength = strlen((CHAR *)szKey);
	while (i < nKeyLength)
	{
		if (j == -1 || szKey[i] == szKey[j])
		{
			i++;
			j++;
			if (szKey[i] != szKey[j])
			{
				if (i < nNextLength)
					pNext[i] = j;
				else
					return false;
			}
			else
			{
				if (i < nNextLength)
					pNext[i] = pNext[j];
				else
					return false;
			}
		}
		else
		{
			j = pNext[j];
		}
	}
	return true;
}
//KMP字符串匹配算法
//返回值大于0,匹配成功
//返回值小于0,匹配失败
int KMP_StrFind(BYTE *szSource, int nSourceLength, BYTE *szKey, int nKeyLength)
{
	int Next[1024] = { 0 };
	int i = 0, j = 0;
	if (nSourceLength <= 0)
		nSourceLength = strlen((CHAR *)szSource);
	if (nKeyLength <= 0)
		nKeyLength = strlen((CHAR *)szKey);
	if (!GetNext(szSource, nKeyLength, Next, 1024))
		return -1;
	while (i < nSourceLength && j < nKeyLength)
	{
		if (j == -1 || szSource[i] == szKey[j])
		{
			i++;
			j++;
		}
		else
		{ //若发生不匹配,模式串向右滑动,继续与父串匹配,父串指针无须回溯
			j = Next[j];
		}
	}
	if (j == nKeyLength)
	{ //匹配成功,返回位置
		return i - nKeyLength;
	}
	else
		return -1;
}


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

typedef  unsigned int UINT;
typedef  unsigned char BYTE;
typedef  unsigned long DWORD;

UINT Ue(BYTE *pBuff, UINT nLen, UINT &nStartBit)
{
	//计算0bit的个数
	UINT nZeroNum = 0;
	while (nStartBit < nLen * 8)
	{
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) //&:按位与，%取余
		{
			break;
		}
		nZeroNum++;
		nStartBit++;
	}
	nStartBit++;


	//计算结果
	DWORD dwRet = 0;
	for (UINT i = 0; i<nZeroNum; i++)
	{
		dwRet <<= 1;
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			dwRet += 1;
		}
		nStartBit++;
	}
	return (1 << nZeroNum) - 1 + dwRet;
}


int Se(BYTE *pBuff, UINT nLen, UINT &nStartBit)
{
	int UeVal = Ue(pBuff, nLen, nStartBit);
	double k = UeVal;
	int nValue = ceil(k / 2);//ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00
	if (UeVal % 2 == 0)
		nValue = -nValue;
	return nValue;
}


DWORD u(UINT BitCount, BYTE * buf, UINT &nStartBit)
{
	DWORD dwRet = 0;
	for (UINT i = 0; i<BitCount; i++)
	{
		dwRet <<= 1;
		if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			dwRet += 1;
		}
		nStartBit++;
	}
	return dwRet;
}

/**
* H264的NAL起始码防竞争机制
*
* @param buf SPS数据内容
*
* @无返回值
*/
void de_emulation_prevention(BYTE* buf, unsigned int* buf_size)
{
	int i = 0, j = 0;
	BYTE* tmp_ptr = NULL;
	unsigned int tmp_buf_size = 0;
	int val = 0;

	tmp_ptr = buf;
	tmp_buf_size = *buf_size;
	for (i = 0; i<(tmp_buf_size - 2); i++)
	{
		//check for 0x000003
		val = (tmp_ptr[i] ^ 0x00) + (tmp_ptr[i + 1] ^ 0x00) + (tmp_ptr[i + 2] ^ 0x03);
		if (val == 0)
		{
			//kick out 0x03
			for (j = i + 2; j<tmp_buf_size - 1; j++)
				tmp_ptr[j] = tmp_ptr[j + 1];

			//and so we should devrease bufsize
			(*buf_size)--;
		}
	}
}

/**
* 解码SPS,获取视频图像宽、高和帧率信息
*
* @param buf SPS数据内容
* @param nLen SPS数据的长度
* @param width 图像宽度
* @param height 图像高度
* @成功则返回true , 失败则返回false
*/
bool h264_decode_sps(BYTE * buf, unsigned int nLen, int &width, int &height, int &fps)
{
	UINT StartBit = 0;
	fps = 0;
	de_emulation_prevention(buf, &nLen);

	int forbidden_zero_bit = u(1, buf, StartBit);
	int nal_ref_idc = u(2, buf, StartBit);
	int nal_unit_type = u(5, buf, StartBit);
	if (nal_unit_type == 7)
	{
		int profile_idc = u(8, buf, StartBit);
		int constraint_set0_flag = u(1, buf, StartBit);//(buf[1] & 0x80)>>7;
		int constraint_set1_flag = u(1, buf, StartBit);//(buf[1] & 0x40)>>6;
		int constraint_set2_flag = u(1, buf, StartBit);//(buf[1] & 0x20)>>5;
		int constraint_set3_flag = u(1, buf, StartBit);//(buf[1] & 0x10)>>4;
		int reserved_zero_4bits = u(4, buf, StartBit);
		int level_idc = u(8, buf, StartBit);

		int seq_parameter_set_id = Ue(buf, nLen, StartBit);

		if (profile_idc == 100 || profile_idc == 110 ||
			profile_idc == 122 || profile_idc == 144)
		{
			int chroma_format_idc = Ue(buf, nLen, StartBit);
			if (chroma_format_idc == 3)
				int residual_colour_transform_flag = u(1, buf, StartBit);
			int bit_depth_luma_minus8 = Ue(buf, nLen, StartBit);
			int bit_depth_chroma_minus8 = Ue(buf, nLen, StartBit);
			int qpprime_y_zero_transform_bypass_flag = u(1, buf, StartBit);
			int seq_scaling_matrix_present_flag = u(1, buf, StartBit);

			int seq_scaling_list_present_flag[8];
			if (seq_scaling_matrix_present_flag)
			{
				for (int i = 0; i < 8; i++) {
					seq_scaling_list_present_flag[i] = u(1, buf, StartBit);
				}
			}
		}
		int log2_max_frame_num_minus4 = Ue(buf, nLen, StartBit);
		int pic_order_cnt_type = Ue(buf, nLen, StartBit);
		if (pic_order_cnt_type == 0)
			int log2_max_pic_order_cnt_lsb_minus4 = Ue(buf, nLen, StartBit);
		else if (pic_order_cnt_type == 1)
		{
			int delta_pic_order_always_zero_flag = u(1, buf, StartBit);
			int offset_for_non_ref_pic = Se(buf, nLen, StartBit);
			int offset_for_top_to_bottom_field = Se(buf, nLen, StartBit);
			int num_ref_frames_in_pic_order_cnt_cycle = Ue(buf, nLen, StartBit);

			int *offset_for_ref_frame = new int[num_ref_frames_in_pic_order_cnt_cycle];
			for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
				offset_for_ref_frame[i] = Se(buf, nLen, StartBit);
			delete[] offset_for_ref_frame;
		}
		int num_ref_frames = Ue(buf, nLen, StartBit);
		int gaps_in_frame_num_value_allowed_flag = u(1, buf, StartBit);
		int pic_width_in_mbs_minus1 = Ue(buf, nLen, StartBit);
		int pic_height_in_map_units_minus1 = Ue(buf, nLen, StartBit);

		width = (pic_width_in_mbs_minus1 + 1) * 16;
		height = (pic_height_in_map_units_minus1 + 1) * 16;

		int frame_mbs_only_flag = u(1, buf, StartBit);
		if (!frame_mbs_only_flag)
			int mb_adaptive_frame_field_flag = u(1, buf, StartBit);

		int direct_8x8_inference_flag = u(1, buf, StartBit);
		int frame_cropping_flag = u(1, buf, StartBit);
		if (frame_cropping_flag)
		{
			int frame_crop_left_offset = Ue(buf, nLen, StartBit);
			int frame_crop_right_offset = Ue(buf, nLen, StartBit);
			int frame_crop_top_offset = Ue(buf, nLen, StartBit);
			int frame_crop_bottom_offset = Ue(buf, nLen, StartBit);
		}
		int vui_parameter_present_flag = u(1, buf, StartBit);
		if (vui_parameter_present_flag)
		{
			int aspect_ratio_info_present_flag = u(1, buf, StartBit);
			if (aspect_ratio_info_present_flag)
			{
				int aspect_ratio_idc = u(8, buf, StartBit);
				if (aspect_ratio_idc == 255)
				{
					int sar_width = u(16, buf, StartBit);
					int sar_height = u(16, buf, StartBit);
				}
			}
			int overscan_info_present_flag = u(1, buf, StartBit);
			if (overscan_info_present_flag)
				int overscan_appropriate_flagu = u(1, buf, StartBit);
			int video_signal_type_present_flag = u(1, buf, StartBit);
			if (video_signal_type_present_flag)
			{
				int video_format = u(3, buf, StartBit);
				int video_full_range_flag = u(1, buf, StartBit);
				int colour_description_present_flag = u(1, buf, StartBit);
				if (colour_description_present_flag)
				{
					int colour_primaries = u(8, buf, StartBit);
					int transfer_characteristics = u(8, buf, StartBit);
					int matrix_coefficients = u(8, buf, StartBit);
				}
			}
			int chroma_loc_info_present_flag = u(1, buf, StartBit);
			if (chroma_loc_info_present_flag)
			{
				int chroma_sample_loc_type_top_field = Ue(buf, nLen, StartBit);
				int chroma_sample_loc_type_bottom_field = Ue(buf, nLen, StartBit);
			}
			int timing_info_present_flag = u(1, buf, StartBit);

			if (timing_info_present_flag)
			{
				int num_units_in_tick = u(32, buf, StartBit);
				int time_scale = u(32, buf, StartBit);
				fps = time_scale / num_units_in_tick;
				int fixed_frame_rate_flag = u(1, buf, StartBit);
				if (fixed_frame_rate_flag)
				{
					fps = fps / 2;
				}
			}
		}
		return true;
	}
	else
		return false;
}
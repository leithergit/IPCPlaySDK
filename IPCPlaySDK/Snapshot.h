#pragma once
#include "Common.h"

/// @brief  截图类开，且于FFMPEG解码的帧的截图,目前只支持jpeg和bmp两种格式的截图
/// @code 详见 CDvoPlayer::SnapShot(IN CHAR *szFileName, IN SNAPSHOT_FORMAT nFileFormat = XIFF_JPG)函数
class CSnapshot
{
	AVFrame* pAvFrame;
	byte*	 pImage;
	int		 nImaageSize;
	int		 nVideoWidth;
	int		 nVideoHeight;
	AVPixelFormat nAvFormat;
	AVCodecID nCodecID;
public:
	CSnapshot(AVPixelFormat nAvFormat, int nWidth, int nHeight)
	{
		ZeroMemory(this, sizeof(CSnapshot));
		pAvFrame = av_frame_alloc();
		int nImageSize = av_image_get_buffer_size(nAvFormat, nWidth, nHeight, 16);
		if (nImageSize < 0)
			return;
		pImage = (byte *)_aligned_malloc(nImageSize, 32);
		if (!pImage)
		{
			return;
		}
		// 把显示图像与YUV帧关联
		av_image_fill_arrays(pAvFrame->data, pAvFrame->linesize, pImage, nAvFormat, nWidth, nHeight, 16);
		pAvFrame->width = nWidth;
		pAvFrame->height = nHeight;
		pAvFrame->format = nAvFormat;
		nVideoHeight = nHeight;
		nVideoWidth = nWidth;
		this->nAvFormat = nAvFormat;
	}
	~CSnapshot()
	{
		if (pImage)
		{
			_aligned_free(pImage);
			pImage = NULL;
		}
		av_free(pAvFrame);
		pAvFrame = NULL;
	}

	inline AVPixelFormat GetPixelFormat()
	{
		return nAvFormat;
	}
	inline int SetCodecID(IPC_CODEC nVideoCodec)
	{
		switch (nVideoCodec)
		{
		case CODEC_H264:
			nCodecID = AV_CODEC_ID_H264;
			break;
		case CODEC_H265:
			nCodecID = AV_CODEC_ID_H265;
			break;
		default:
		{
			return -1;
			break;
		}
		}
		return IPC_Succeed;
	}

	/// @brief 把Dxva硬解码帧转换成YUV420帧
	int CopyDxvaFrame(AVFrame *pAvFrameDXVA)
	{
		if (!pAvFrameDXVA || !pAvFrame)
			return -1;

		if (pAvFrameDXVA->format != AV_PIX_FMT_DXVA2_VLD)
			return -1;

		IDirect3DSurface9* pSurface = (IDirect3DSurface9 *)pAvFrameDXVA->data[3];
		D3DLOCKED_RECT lRect;
		D3DSURFACE_DESC SurfaceDesc;
		pSurface->GetDesc(&SurfaceDesc);
		HRESULT hr = pSurface->LockRect(&lRect, nullptr, D3DLOCK_READONLY);
		if (FAILED(hr))
		{
			//OutputMsg("%s IDirect3DSurface9::LockRect failed:hr = %08.\n", __FUNCTION__, hr);
			return -1;
		}

		// Y分量图像
		byte *pSrcY = (byte *)lRect.pBits;
		// UV分量图像
		//byte *pSrcUV = (byte *)lRect.pBits + lRect.Pitch * SurfaceDesc.Height;
		byte *pSrcUV = (byte *)lRect.pBits + lRect.Pitch * CVideoDecoder::GetAlignedDimension(nCodecID, pAvFrameDXVA->height);

		byte* dstY = pAvFrame->data[0];
		byte* dstU = pAvFrame->data[1];
		byte* dstV = pAvFrame->data[2];

		UINT heithtUV = pAvFrameDXVA->height / 2;
		UINT widthUV = pAvFrameDXVA->width / 2;

		// 复制Y分量
		for (int i = 0; i < pAvFrameDXVA->height; i++)
			memcpy(&dstY[i*pAvFrameDXVA->width], &pSrcY[i*lRect.Pitch], pAvFrameDXVA->width);

		// 复制VU分量
		for (int i = 0; i < heithtUV; i++)
		{
			for (int j = 0; j < widthUV; j++)
			{
				dstU[i*widthUV + j] = pSrcUV[i*lRect.Pitch + 2 * j];
				dstV[i*widthUV + j] = pSrcUV[i*lRect.Pitch + 2 * j + 1];
			}
		}

		pSurface->UnlockRect();
		av_frame_copy_props(pAvFrame, pAvFrameDXVA);
		pAvFrame->format = AV_PIX_FMT_YUV420P;
		return 0;
	}

	int CopyFrame(AVFrame *pFrame)
	{
		if ((pFrame && pAvFrame))
		{
			av_frame_copy(pAvFrame, pFrame);
			av_frame_copy_props(pAvFrame, pFrame);
			return 0;
		}
		else
			return -1;
	}

	bool SaveJpeg(char *szJpegFile)
	{
		AVFormatContext* pJpegFormatCtx = nullptr;
		AVOutputFormat* fmt = nullptr;
		AVStream* JpegStream = nullptr;
		AVCodecContext* pJpegCodecCtx = nullptr;
		AVCodec* pCodec = nullptr;
		AVPacket pkt;
		ZeroMemory(&pkt, sizeof(AVPacket));
		int got_picture = 0;
		int nAvError = 0;
		char szAvError[1024];
		__try
		{
			pJpegFormatCtx = avformat_alloc_context();
			//Guess format
			fmt = av_guess_format("mjpeg", NULL, NULL);
			pJpegFormatCtx->oformat = fmt;
			//Output URL
			if (avio_open(&pJpegFormatCtx->pb, szJpegFile, AVIO_FLAG_READ_WRITE) < 0)
			{
				TraceMsg("Couldn't open output file.");
				__leave;
			}

			fmt = pJpegFormatCtx->oformat;
			JpegStream = avformat_new_stream(pJpegFormatCtx, 0);
			if (JpegStream == NULL){
				__leave;
			}
			pCodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
			if (!pCodec)
			{
				TraceMsg("Codec not found.");
				__leave;
			}
			pJpegCodecCtx = avcodec_alloc_context3(pCodec);
			if (pJpegCodecCtx)
				avcodec_parameters_to_context(pJpegCodecCtx, JpegStream->codecpar);
			else
			{
				TraceMsg("%s avcodec_alloc_context3 Failed.\n", __FUNCTION__);
				__leave;
			}

			pJpegCodecCtx->codec_id = fmt->video_codec;
			pJpegCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
			pJpegCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;//(AVPixelFormat)pAvFrame->format;
			pJpegCodecCtx->width = pAvFrame->width;
			pJpegCodecCtx->height = pAvFrame->height;
			pJpegCodecCtx->time_base.num = 1;
			pJpegCodecCtx->time_base.den = 25;
			//Output some information
			av_dump_format(pJpegFormatCtx, 0, szJpegFile, 1);

			if (nAvError = avcodec_open2(pJpegCodecCtx, pCodec, NULL) < 0)
			{
				av_strerror(nAvError, szAvError, 1024);
				TraceMsg("Could not open codec:%s.", szAvError);
				__leave;
			}
			//Write Header
			avformat_write_header(pJpegFormatCtx, NULL);

			int y_size = pJpegCodecCtx->width * pJpegCodecCtx->height;

			av_new_packet(&pkt, y_size * 3);
			//Encode
			int ret = avcodec_encode_video2(pJpegCodecCtx, &pkt, pAvFrame, &got_picture);
			if (ret < 0)
			{
				TraceMsg("Encode Error.\n");
				__leave;
			}
			if (got_picture == 1)
			{
				pkt.stream_index = JpegStream->index;
				ret = av_write_frame(pJpegFormatCtx, &pkt);
				av_write_trailer(pJpegFormatCtx);
			}
		}
		__finally
		{
			if (pkt.buf)
				av_packet_unref(&pkt);

			if (JpegStream)
				avcodec_parameters_free(&JpegStream->codecpar);
			if (pJpegCodecCtx)
				avcodec_close(pJpegCodecCtx);
			if (pJpegFormatCtx)
			{
				avio_close(pJpegFormatCtx->pb);
				avformat_free_context(pJpegFormatCtx);
			}
		}
		return true;
	}

	// 
	bool CreateBmp(const char *filename, uint8_t *pRGBBuffer, int width, int height, int bpp)
	{
		BITMAPFILEHEADER bmpheader;
		BITMAPINFOHEADER bmpinfo;
		FILE *fp = NULL;

		fp = fopen(filename, "wb");
		if (fp == NULL)
		{
			return false;
		}

		bmpheader.bfType = ('M' << 8) | 'B';
		bmpheader.bfReserved1 = 0;
		bmpheader.bfReserved2 = 0;
		bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		bmpheader.bfSize = bmpheader.bfOffBits + width*height*bpp / 8;

		bmpinfo.biSize = sizeof(BITMAPINFOHEADER);
		bmpinfo.biWidth = width;
		bmpinfo.biHeight = 0 - height;
		bmpinfo.biPlanes = 1;
		bmpinfo.biBitCount = bpp;
		bmpinfo.biCompression = BI_RGB;
		bmpinfo.biSizeImage = 0;
		bmpinfo.biXPelsPerMeter = 100;
		bmpinfo.biYPelsPerMeter = 100;
		bmpinfo.biClrUsed = 0;
		bmpinfo.biClrImportant = 0;

		fwrite(&bmpheader, sizeof(BITMAPFILEHEADER), 1, fp);
		fwrite(&bmpinfo, sizeof(BITMAPINFOHEADER), 1, fp);
		fwrite(pRGBBuffer, width*height*bpp / 8, 1, fp);
		fclose(fp);
		fp = NULL;
		return true;
	}


	bool SaveBmp(char *szBmpFile)
	{
		shared_ptr<PixelConvert> pVideoScale = make_shared< PixelConvert>(pAvFrame, D3DFMT_R8G8B8, GQ_BICUBIC);
		pVideoScale->ConvertPixel(pAvFrame);
		return CreateBmp(szBmpFile, (uint8_t *)pVideoScale->pImage, pAvFrame->width, pAvFrame->height, 24);
	}
};
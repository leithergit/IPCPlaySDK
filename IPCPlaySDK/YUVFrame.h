#pragma once
#include "Common.h"


struct FrameYV12
{
	int nWidth;
	int nHeight;
	int nStrideY;
	int nStrideUV;
	byte *pY;
	byte *pU;
	byte *pV;

	FrameYV12()
	{
		ZeroMemory(this, sizeof(FrameYV12));
	}
	FrameYV12(const unsigned char* pY,
		const unsigned char* pU,
		const unsigned char* pV,
		int nStrideY,
		int nStrideUV,
		int nWidth,
		int nHeight,
		INT64 nTime)
	{
		int nYSize = nStrideY*nHeight;
		int nUVSize = nStrideUV*nHeight;
		this->pY = new byte[nYSize];
		this->pU = new byte[nUVSize / 2];
		this->pV = new byte[nUVSize / 2];

		// 复制Y分量
		for (int i = 0; i < nHeight; i++)
			memcpy(&this->pY[i*nWidth], &pY[i*nStrideY], nWidth);

		// 复制VU分量
		for (int i = 0; i < nHeight / 2; i++)
		{
			memcpy(&this->pU[i*nWidth], &pU[i*nStrideUV], nWidth);
			memcpy(&this->pV[i*nWidth], &pV[i*nStrideUV], nWidth);
		}

		this->nHeight = nHeight;
		this->nWidth = nWidth;
		this->nStrideUV = nStrideUV;
		this->nStrideY = nStrideY;
	}
	FrameYV12(AVFrame *pFrame, INT64 nTime)
	{
		nStrideY = pFrame->linesize[0];
		nStrideUV = pFrame->linesize[1];
		nHeight = pFrame->height;
		nWidth = pFrame->width;

		pY = new byte[nStrideY*nHeight];
		pU = new byte[nStrideUV*nHeight >> 1];
		pV = new byte[nStrideUV*nHeight >> 1];

		memcpy(pY, pFrame->data[0], nStrideY*nHeight);
		memcpy(pU, pFrame->data[1], nStrideUV*nHeight >> 1);
		memcpy(pY, pFrame->data[2], nStrideUV*nHeight >> 1);
	}
	~FrameYV12()
	{
		if (pY)
			delete[]pY;
		if (pU)
			delete[]pU;
		// 		if (pV)
		// 			delete []pV;
		ZeroMemory(this, sizeof(FrameYV12));
	}

};
struct FrameNV12
{
	int nWidth;
	int nHeight;
	int nStrideY;
	int nStrideUV;
	int nSurfaceHeight;
	int nSurfaceWidth;
	byte *pNV12Surface;

	FrameNV12()
	{
		ZeroMemory(this, sizeof(FrameNV12));
	}
	FrameNV12(AVFrame *pAvFrame, INT64 nTime)
	{
		ZeroMemory(this, sizeof(FrameNV12));

		if (pAvFrame->format != AV_PIX_FMT_DXVA2_VLD)
			return;

		IDirect3DSurface9* pSurface = (IDirect3DSurface9 *)pAvFrame->data[3];
		D3DLOCKED_RECT lRect;
		D3DSURFACE_DESC SurfaceDesc;
		pSurface->GetDesc(&SurfaceDesc);
		HRESULT hr = pSurface->LockRect(&lRect, nullptr, D3DLOCK_READONLY);
		if (FAILED(hr))
		{
			TraceMsgA("%s IDirect3DSurface9::LockRect failed:hr = %08.\n", __FUNCTION__, hr);
			return;
		}
		pNV12Surface = new byte[lRect.Pitch*SurfaceDesc.Height * 3 / 2];
		memcpy(pNV12Surface, lRect.pBits, lRect.Pitch*SurfaceDesc.Height * 3 / 2);
		nStrideUV = lRect.Pitch / 2;
		nStrideY = lRect.Pitch;
		nSurfaceHeight = SurfaceDesc.Height;
		nSurfaceWidth = SurfaceDesc.Width;
		pSurface->UnlockRect();

		nHeight = pAvFrame->height;
		nWidth = pAvFrame->width;

	}
	~FrameNV12()
	{
		if (pNV12Surface)
			delete[]pNV12Surface;

		ZeroMemory(this, sizeof(FrameNV12));
	}

};
typedef shared_ptr<FrameNV12> FrameNV12Ptr;
typedef shared_ptr<FrameYV12> FrameYV12Ptr;

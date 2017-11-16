#pragma once
#include "DirectDraw.h"
class CDisplayYUVFrame
{
private:
	byte *pYUV;
	int nYSize;
	int nUVSize;
	int nStrideY;
	int nStrideUV;
	int nWidth;
	int nHeight;
	int	nPictureSize;
	boost::shared_ptr<CDirectDraw>m_pDDraw;
	boost::shared_ptr<ImageSpace> m_pYUVImage;
public:
	CDisplayYUVFrame(const unsigned char* pY,
		const unsigned char* pU,
		const unsigned char* pV,
		int nStrideY,
		int nStrideUV,
		int nWidth,
		int nHeight)
	{
		ZeroMemory(this, sizeof(CDisplayYUVFrame));
		if (!pYUV)
		{
			nYSize = nWidth * nHeight;
			nUVSize = (nWidth>>1)*nHeight;
			pYUV = (byte *)_aligned_malloc(nYSize + nUVSize, 16);
			this->nWidth = nWidth;
			this->nHeight = nHeight;
			this->nStrideY = nStrideY;
			this->nStrideUV = nStrideUV;
			nPictureSize = nStrideY * nHeight;
		}
		UpdateYUV(pY, pU, pV);
	}
	~CDisplayYUVFrame()
	{
		if (pYUV)
			_aligned_free(pYUV);
	}
	void UpdateYUV(const unsigned char* pY,
		const unsigned char* pU,
		const unsigned char* pV)
	{
		// 复制Y分量
		for (int i = 0; i < nHeight; i++)
			memcpy(&this->pYUV[i*nWidth], &pY[i*nStrideY], nWidth);

		byte *pU1 = &pYUV[nYSize];
		byte *pV1 = &pYUV[nYSize + nUVSize >> 1];
		// 复制VU分量
		for (int i = 0; i < (nHeight >>1); i++)
		{
			memcpy(&pU1[i*(nWidth >>1)], &pU[i*nStrideUV], nWidth);
			memcpy(&pV1[i*(nWidth >>1)], &pV[i*nStrideUV], nWidth);
		}

	}
	inline byte * GetY()
	{
		return pYUV;
	}
	inline byte * GetU()
	{
		return &pYUV[nYSize];
	}
	inline byte * GetV()
	{
		return &pYUV[nYSize + nUVSize / 2];
	}
	bool InitizlizeDisplay(HWND hWnd,int nWidth, int nHeight)
	{
		if (!m_pDDraw)
			m_pDDraw = boost::make_shared<CDirectDraw>();
		if (m_pDDraw)
		{
			//构造DirectDraw表面  
			DDSURFACEDESC2 ddsd = { 0 };
			FormatYV12::Build(ddsd, nWidth, nHeight);

			m_pDDraw->Create<FormatYV12>(hWnd, ddsd);
			m_pYUVImage = boost::make_shared<ImageSpace>();
			m_pYUVImage->dwLineSize[0] = nWidth;
			m_pYUVImage->dwLineSize[1] = nWidth >> 1;
			m_pYUVImage->dwLineSize[2] = nWidth >> 1;
			return true;
		}
		else
			return false;
	}
	void Render(HWND hWnd)
	{
		if (!m_pDDraw)
			InitizlizeDisplay(hWnd,nWidth, nHeight);
		{
			m_pYUVImage->pBuffer[0] = (PBYTE)GetY();
			m_pYUVImage->pBuffer[1] = (PBYTE)GetU();
			m_pYUVImage->pBuffer[2] = (PBYTE)GetV();
			m_pYUVImage->dwLineSize[0] = nStrideY;
			m_pYUVImage->dwLineSize[1] = nStrideUV;
			m_pYUVImage->dwLineSize[2] = nStrideUV;
			m_pDDraw->Draw(*m_pYUVImage, nullptr, nullptr, true);
		}
	}
};

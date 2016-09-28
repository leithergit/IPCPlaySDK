#pragma once
#include "Common.h"

struct RenderUnit
{
	HWND hRenderWnd;
	CDirectDraw* pDDraw;
	int nVideoWidth;
	int nVideoHeight;
	bool bEnableHaccel;
	RenderUnit(HWND hRenderWnd, int nWidth, int nHeight, bool bEnableHaccel)
	{
		ZeroMemory(this, sizeof(RenderUnit));
		this->hRenderWnd = hRenderWnd;
		this->bEnableHaccel = bEnableHaccel;
		nVideoHeight = nHeight;
		nVideoWidth = nWidth;

		if (nHeight && nWidth)
			pDDraw = new CDirectDraw();
		if (!pDDraw)
			return;
		DDSURFACEDESC2 ddsd = { 0 };
		if (bEnableHaccel)
		{
			FormatNV12::Build(ddsd, nWidth, nHeight);
			pDDraw->Create<FormatNV12>(hRenderWnd, ddsd);
		}
		else
		{
			FormatYV12::Build(ddsd, nWidth, nHeight);
			pDDraw->Create<FormatYV12>(hRenderWnd, ddsd);
		}
	}

	void SetVideoSize(int nWidth, int nHeight)
	{
		nVideoHeight = nHeight;
		nVideoWidth = nWidth;
	}

	~RenderUnit()
	{
		if (pDDraw)
		{
			delete pDDraw;
			pDDraw = nullptr;
		}
	}

	HRESULT RenderImage(ImageSpace &YUVImage, RECT *pRectClip, RECT *pRectRender, bool bPresent = true)
	{
		if (!hRenderWnd)
			return E_FAIL;
		if (!nVideoWidth || !nVideoHeight)
		{
			assert(false);
		}
		if (!pDDraw)
		{
			DDSURFACEDESC2 ddsd = { 0 };
			pDDraw = new CDirectDraw();
			if (!pDDraw)
			{
				assert(false);
				return E_FAIL;
			}
			if (bEnableHaccel)
			{
				FormatNV12::Build(ddsd, nVideoWidth, nVideoHeight);
				pDDraw->Create<FormatNV12>(hRenderWnd, ddsd);
			}
			else
			{
				FormatYV12::Build(ddsd, nVideoWidth, nVideoHeight);
				pDDraw->Create<FormatYV12>(hRenderWnd, ddsd);
			}
		}
		return pDDraw->Draw(YUVImage, pRectClip, pRectRender, bPresent);
	}
};

typedef shared_ptr<RenderUnit>RenderUnitPtr;
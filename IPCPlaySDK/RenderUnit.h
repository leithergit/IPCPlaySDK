#pragma once

#include "Common.h"

struct RenderWnd
{
	RenderWnd(){};
	RenderWnd(HWND hWnd, LPRECT prtBorder, bool bPercent)
	{
		ZeroMemory(this, sizeof(RenderWnd));
		hRenderWnd = hWnd;
		SetBorder(prtBorder, bPercent);
	}
	void inline SetBorder(LPRECT prtBorder, bool bPercent = false)
	{
		if (prtBorder)
		{
			if (this->pRtBorder)
				delete this->pRtBorder;
			this->pRtBorder = new RECT;
			CopyRect(this->pRtBorder, prtBorder);
			this->bPercent = bPercent;
		}
		else
		{
			if (this->pRtBorder)
				delete this->pRtBorder;
			this->pRtBorder = nullptr;
		}
	}
	~RenderWnd()
	{
		if (pRtBorder)
			delete pRtBorder;
	}
	HWND	hRenderWnd;
	RECT	*pRtBorder;
	bool	bPercent;
};
typedef shared_ptr<RenderWnd> RenderWndPtr;
#ifndef WIN64
struct RenderUnit :public RenderWnd
{
// private:
//  	RenderUnit(){}
public:
	CDirectDraw* pDDraw;
	int nVideoWidth;
	int nVideoHeight;
	bool bEnableHaccel;

	RenderUnit(HWND hRenderWnd, LPRECT prtBorder, bool bEnableHaccel)
	{
		ZeroMemory(this, sizeof(RenderUnit));
		this->hRenderWnd = hRenderWnd;
		this->bEnableHaccel = bEnableHaccel;
		if (nullptr == prtBorder)
		{
			return;
		}
		nVideoHeight = abs(prtBorder->top - prtBorder->bottom);
		nVideoWidth = abs(prtBorder->left - prtBorder->right);

		if (nVideoHeight && nVideoWidth)
			pDDraw = new CDirectDraw();
		if (!pDDraw)
			return;
		DDSURFACEDESC2 ddsd = { 0 };
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

	void ReInitialize(int nWidth, int nHeight)
	{
		nVideoHeight = nHeight;
		nVideoWidth = nWidth;
		if (!pDDraw)
		{
			pDDraw = new CDirectDraw();
			if (!pDDraw)
				return;
			DDSURFACEDESC2 ddsd = { 0 };
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
		if (!IsNeedRender(hRenderWnd))
			return S_OK;
		if (IsWindowPall(hRenderWnd))
			return S_OK;
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
#endif
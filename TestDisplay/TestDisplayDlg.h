
// TestDisplayDlg.h : 头文件
//

#pragma once
#include <memory>
using namespace std;
using namespace std::tr1;

#include "../IPCPlaySDK/DirectDraw.h"
#include "../IPCPlaySDK/DxSurface/DxSurface.h"
#include <assert.h>

// CTestDisplayDlg 对话框
class CTestDisplayDlg : public CDialogEx
{
// 构造
public:
	CTestDisplayDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_TESTDISPLAY_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
;
	afx_msg void OnBnClickedButtonRender();
	volatile bool bThreadRun = false;
	HANDLE m_hRenderThread1 = nullptr;
	HANDLE m_hRenderThread2 = nullptr
	static UINT __stdcall RenderThread1(void *p)
	{
		CTestDisplayDlg *pThis = (CTestDisplayDlg *)p;
		DDSURFACEDESC2 ddsd = { 0 };
		int nWidth = 1280;
		int nHeight = 720;
		CDirectDraw *m_pDDraw = new CDirectDraw();
		FormatYV12::Build(ddsd, nWidth, nHeight);
		HWND hRenderhWnd = pThis->GetDlgItem(IDC_STATIC_FRAME1)->m_hWnd;
		m_pDDraw->Create<FormatYV12>(hRenderhWnd, ddsd);
		shared_ptr<ImageSpace> m_pYUVImage = NULL;
		m_pYUVImage = make_shared<ImageSpace>();
		m_pYUVImage->dwLineSize[0] = nWidth;
		m_pYUVImage->dwLineSize[1] = nWidth >> 1;
		m_pYUVImage->dwLineSize[2] = nWidth >> 1;
		
		int nSize = nHeight*nWidth * 3 / 2;
		byte *pBuffer = new byte[nSize];
		CFile YuvFile(_T("D:\\Git\\IPCPlaySDK\\axisyuv\\axis1080p_1.yuv"), CFile::modeRead);		
		YuvFile.Read(pBuffer, nSize);
		m_pYUVImage->pBuffer[0] = (PBYTE)pBuffer;
		m_pYUVImage->pBuffer[1] = (PBYTE)pBuffer + nWidth * nHeight;
		m_pYUVImage->pBuffer[2] = (PBYTE)pBuffer + nWidth * 5*nHeight/4;
		long nCount = 0;
		while (pThis->bThreadRun)
		{
			m_pDDraw->Draw(*m_pYUVImage, nullptr, nullptr, true);
			nCount++;
		}
		pThis->SetDlgItemInt(IDC_STATIC_RENDER1, nCount);
		return 0;
	}
	static void CopyFrameYUV420P(D3DLOCKED_RECT *pD3DRect, int nDescHeight, byte *pFrame420P,int nWidth,int nHeight)
	{
		byte *pDest = (byte *)pD3DRect->pBits;
		int nStride = pD3DRect->Pitch;
		int nSize = nDescHeight * nStride;
		int nHalfSize = (nSize) >> 1;
		byte *pDestY = pDest;										// Y分量起始地址
		byte *pDestV = pDest + nSize;								// U分量起始地址
		int nSizeofV = nHalfSize >> 1;
		byte *pDestU = pDestV + (size_t)(nHalfSize >> 1);			// V分量起始地址
		int nSizoefU = nHalfSize >> 1;

		byte *pSrcY = pFrame420P ;
		byte *pSrcU = pFrame420P + nSize;
		byte *pSrcV = pSrcU + (nHalfSize >> 1);

		// YUV420P的U和V分量对调，便成为YV12格式
		// 复制Y分量
		for (int i = 0; i < nHeight; i++)
			memcpy_s(pDestY + i * nStride, nSize * 3 / 2 - i*nStride, pFrame420P + i * nWidth, nWidth);

		// 复制YUV420P的U分量到目村的YV12的U分量
		for (int i = 0; i < nHeight / 2; i++)
			memcpy_s(pDestU + i * nStride / 2, nSizoefU - i*nStride / 2, pSrcU + i * nWidth/2, nWidth / 2);
		
		// 复制YUV420P的V分量到目村的YV12的V分量
		for (int i = 0; i < nHeight / 2; i++)
			memcpy_s(pDestV + i * nStride / 2, nSizeofV - i*nStride / 2, pSrcV + i * nWidth/2, nWidth / 2);
			
	}
	static UINT __stdcall RenderThread2(void *p)
	{
		CTestDisplayDlg *pThis = (CTestDisplayDlg *)p;
		CDxSurfaceEx* m_pDxSurface = new CDxSurfaceEx();
		m_pDxSurface->DisableVsync();		// 禁用垂直同步，播放帧才有可能超过显示器的刷新率，从而达到高速播放的目的
		int nWidth = 1280;
		int nHeight = 720;
		HWND hRenderhWnd = pThis->GetDlgItem(IDC_STATIC_FRAME2)->m_hWnd;
		if (!m_pDxSurface->InitD3D(hRenderhWnd,
			nWidth,
			nHeight,
			true,
			(D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2')))
		{
			assert(false);
			return false;
		}
		int nSize = nHeight*nWidth * 3 / 2;
		byte *pBuffer = new byte[nSize];
		CFile YuvFile(_T("D:\\Git\\IPCPlaySDK\\axisyuv\\axis1080p_2.yuv"), CFile::modeRead);
		YuvFile.Read(pBuffer, nSize);
		long nCount = 0;
		D3DLOCKED_RECT d3d_rect;
		D3DSURFACE_DESC Desc;
		while (pThis->bThreadRun)
		{	
			HRESULT	hr = m_pDxSurface->m_pDirect3DSurfaceRender->GetDesc(&Desc);
			hr |= m_pDxSurface->m_pDirect3DSurfaceRender->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
			if (FAILED(hr))
			{
				continue;
			}
			
			CopyFrameYUV420P(&d3d_rect, Desc.Height, pBuffer,nWidth,nHeight);

			hr = m_pDxSurface->m_pDirect3DSurfaceRender->UnlockRect();
			if (FAILED(hr))
			{
				//DxTraceMsg("%s line(%d) IDirect3DSurface9::UnlockRect failed:hr = %08X.\n",__FUNCTION__,__LINE__,hr);
				continue;;
			}
			
			IDirect3DSurface9 * pBackSurface = NULL;
			//m_pDirect3DDeviceEx->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
			m_pDxSurface->m_pDirect3DDeviceEx->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
			m_pDxSurface->m_pDirect3DDeviceEx->BeginScene();
			hr = m_pDxSurface->m_pDirect3DDeviceEx->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackSurface);
			
			if (FAILED(hr))
			{
				m_pDxSurface->m_pDirect3DDeviceEx->EndScene();
				//DxTraceMsg("%s line(%d) IDirect3DDevice9Ex::GetBackBuffer failed:hr = %08X.\n",__FUNCTION__,__LINE__,hr);
				continue;
			}
			pBackSurface->GetDesc(&Desc);
			RECT dstrt = { 0, 0, Desc.Width, Desc.Height };
			RECT srcrt = { 0, 0, nWidth , nHeight };
			
			hr = m_pDxSurface->m_pDirect3DDeviceEx->StretchRect(m_pDxSurface->m_pDirect3DSurfaceRender, &srcrt, pBackSurface, &dstrt, D3DTEXF_LINEAR);
			
			pBackSurface->Release();
			m_pDxSurface->m_pDirect3DDeviceEx->EndScene();
			hr |= m_pDxSurface->m_pDirect3DDeviceEx->PresentEx(NULL, NULL,hRenderhWnd , NULL, 0);
			nCount++;
		}
		pThis->SetDlgItemInt(IDC_STATIC_RENDER2, nCount);

		return 0;
	}
	afx_msg void OnBnClickedButtonRender2();
};

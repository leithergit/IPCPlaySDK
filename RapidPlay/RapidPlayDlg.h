
// RapidPlayDlg.h : header file
//

#pragma once
#include "afxcmn.h"
#include "ipcplaysdk.h"
#include "../include/Utility.h"
#include "../include/TimeUtility.h"
#include "../include/decSPS.h"
#include "../include/AutoLock.h"
#include <memory>
#include <process.h>

using namespace std;
using namespace std::tr1;
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

#define Max_Bffer_Size	(512*1024)
class CDHStreamParser
{
	byte *pBuffer;
	int		nBufferSize;
	int		nDataLength;
	CRITICAL_SECTION csBuffer;
	int		nSequence;
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
	}
	~CDHStreamParser()
	{
		::EnterCriticalSection(&csBuffer);
		if (pBuffer)
			delete[] pBuffer;
		nDataLength = 0;
		nBufferSize = 0;
		pBuffer = nullptr;
		::LeaveCriticalSection(&csBuffer);
		::DeleteCriticalSection(&csBuffer);
	}

	int InputStream(byte *pData,int nInputLen)
	{
		CAutoLock lock(&csBuffer);
		if ((nDataLength + nInputLen) < nBufferSize)
		{
			memcpy_s(&pBuffer[nDataLength], nBufferSize - nDataLength, pData, nInputLen);
			nDataLength += nInputLen;
		}
		else
		{
			if (nBufferSize * 2 > Max_Bffer_Size)
				return -1;
			MemMerge((char **)&pBuffer, nDataLength, nBufferSize, (char *)pData, nInputLen);
			return nDataLength;
		}
		return nDataLength;
	}
	DHFrame *ParserFrame()
	{
		CAutoLock Lock(&csBuffer);
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

		DHFrame *pFrame = new DHFrame;
		if (!pFrame)
			return nullptr;
		int extNum = pData[18];
		unsigned char* extptr = pData + 16;
		pFrame->nExtLen = 0;
		while (extNum > 0)
		{
			int exttype = extptr[0] | extptr[1] << 8;
			int extlen = 4 + extptr[2] | extptr[3] << 8;//2字节类型，2字节长度
			pFrame->nExtLen += extlen;
			if (nDataLength < 20 + pFrame->nExtLen)
				return nullptr;
			extptr += extlen;
			extNum--;
		}
		pFrame->nFrameSize = pData[23] << 24 | pData[22] << 16 | pData[21] << 8 | pData[20];
		//pFrame->nFrameSize = pFrame->nFrameSize + 24 + pFrame->nExtLen;
		if (nDataLength < (pFrame->nFrameSize + 24))
		{
			delete pFrame;
			return nullptr;
		}

#ifndef _DEBUG
		pFrame->pContent = new byte[pFrame->nFrameSize];
#else
		pFrame->pContent = new byte[pFrame->nFrameSize + 16];
#endif
		if (!pFrame->pContent)
		{
			delete pFrame;
			return nullptr;
		}
		if (pData[3] == I_FRAME_FLAG)
			pFrame->bKeyFrame = true;
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

#ifdef _DEBUG
		ZeroMemory(pFrame->pContent, pFrame->nFrameSize+16);
#endif
		pData += 24;
		memcpy_s(pFrame->pContent, pFrame->nFrameSize, pData  , pFrame->nFrameSize);
		memmove(pBuffer, pData + pFrame->nFrameSize, nDataLength - (pFrame->nFrameSize + 24 ));
		ZeroMemory(&pBuffer[nDataLength], nBufferSize - nDataLength);
		nDataLength -= (pFrame->nFrameSize + 24);
		return pFrame;
	}
};
// CRapidPlayDlg dialog
class CRapidPlayDlg : public CDialogEx
{
// Construction
public:
	CRapidPlayDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_RAPIDPLAY_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonBrowse();
	CListCtrl m_ctlListFile;
	CString		m_strDirectory;
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonPause();
	afx_msg void OnBnClickedButtonCapture();
	int		m_nCurFileIndex;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	IPC_PLAYHANDLE	m_hPlayHandle;
	static UINT __stdcall ThreadParserFile(void *p)
	{
		CRapidPlayDlg *pThis = (CRapidPlayDlg*)p;
		return pThis->ParserFile();
	}

	bool IsH264KeyFrame(byte *pFrame)
	{
		int RTPHeaderBytes = 0;
		if (pFrame[0] == 0 &&
			pFrame[1] == 0 &&
			pFrame[2] == 0 &&
			pFrame[3] == 1)
		{
			int nal_type = pFrame[4] & 0x1F;
			if (nal_type == 5 || nal_type == 7 || nal_type == 8 || nal_type == 2)
			{
				return true;
			}
		}

		return false;
	}

	bool m_bParserFile = false;
	HANDLE m_hThreadParserFile = nullptr;
	IPC_PLAYHANDLE hPlayHandle = nullptr;
	UINT ParserFile()
	{		
		int nCount = m_ctlListFile.GetItemCount();
		if (m_nCurFileIndex < 0)
		{
			m_nCurFileIndex = 0;
		}
		int nBufferSize = 16 * 1024;
		byte *pFileBuffer = new byte[nBufferSize];
		ZeroMemory(pFileBuffer, nBufferSize);
		
		CString strFile = /*_T("E:\\DVORecord\\TestDAV\\SaveRecord12.23.dav");*/m_strDirectory + _T("\\") + m_ctlListFile.GetItemText(m_nCurFileIndex, 1);
		
		try
		{
			CFile file(strFile, CFile::modeRead | CFile::shareDenyWrite);
			int nWidth = 0, nHeight = 0, nFramerate = 0;
			int nOffsetStart = 0;
			int nOffsetMatched = 0;
			hPlayHandle = ipcplay_OpenRTStream(GetDlgItem(IDC_STATIC_FRAME)->GetSafeHwnd());
			IPC_MEDIAINFO MediaHeader;
			MediaHeader.nVideoCodec = CODEC_H264;
			MediaHeader.nAudioCodec = CODEC_UNKNOWN;
			MediaHeader.nVideoWidth = 1920;
			MediaHeader.nVideoHeight = 1080;
			MediaHeader.nFps = 200;
			ipcplay_SetStreamHeader(hPlayHandle, (byte *)&MediaHeader, sizeof(IPC_MEDIAINFO));
			ipcplay_EnableStreamParser(hPlayHandle, CODEC_H264);

			ipcplay_SetDecodeDelay(hPlayHandle, 0);
			ipcplay_Start(hPlayHandle, false, true, true);
			CDHStreamParser *pDHStream = nullptr;
			DHFrame *pFrame = nullptr;
			while (m_bParserFile)
			{
				int nReadLength = file.Read(pFileBuffer, nBufferSize);
				if (nReadLength > 0)
				{
					if (!pDHStream)
						pDHStream = new CDHStreamParser(pFileBuffer, nReadLength);
					else
						pDHStream->InputStream(pFileBuffer, nReadLength);
								
					do 
					{
						pFrame = pDHStream->ParserFrame();
						if (pFrame)
						{
							bool bKeyFrame = IsH264KeyFrame(pFrame->pContent);
							do
							{
								if (ipcplay_InputDHStream(hPlayHandle, pFrame) == IPC_Succeed)
									break;
								Sleep(10);
							} while (m_bParserFile);
							if (pFrame)
								delete pFrame;
						}
						else
							break;
						
					} while (m_bParserFile);
				}
				else
				{
					file.Close();
					do
					{
						m_nCurFileIndex++;
						strFile = m_strDirectory + _T("\\") + m_ctlListFile.GetItemText(m_nCurFileIndex, 1);
						if (file.Open(strFile, CFile::modeRead))
							break;
					} while (m_bParserFile);
					
				}
				Sleep(10);
			}
			delete pDHStream;
		}
		catch (CFileException* e)
		{
		}
		delete[]pFileBuffer;
		return 0;
	}
	afx_msg void OnBnClickedButtonStop();
	
};

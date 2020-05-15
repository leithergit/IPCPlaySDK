#pragma once
#include "common.h"
#include "AutoLineLock.h"
using namespace std;
using namespace std::tr1;
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
		CAutoLock lock(&csBuffer);
		if ((nDataLength + nInputLen) < nBufferSize)
		{
			memcpy_s(&pBuffer[nDataLength], nBufferSize - nDataLength, pData, nInputLen);
			nDataLength += nInputLen;
		}
		else
		{
			if (nBufferSize * 2 > _MaxBuffSize)
				return IPC_Error_FrameCacheIsFulled;
			MemMerge(( char **)&pBuffer, nDataLength, nBufferSize, ( char *)pData, nInputLen);
			return 0;
		}
		return 0;
	}
	DHFrame *ParserFrame()
	{
		int nFrameSize = 0;
		int nExtLen = 0;
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

		memcpy_s(pFrame->pContent, pFrame->nFrameSize+24, pData, pFrame->nFrameSize + 24);
		memmove(pBuffer, pData + pFrame->nFrameSize, nDataLength - (pFrame->nFrameSize + 24));
		ZeroMemory(&pBuffer[nDataLength], nBufferSize - nDataLength);
		nDataLength -= (pFrame->nFrameSize + 24);
		return pFrame;
	}
};
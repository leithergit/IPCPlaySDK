
#include "Common.h"
#include "Media.h"
#include "IPCPlaySDK.h"
#include "./DxSurface/DxSurface.h"
#include "AutoLock.h"
#include "./VideoDecoder/VideoDecoder.h"
#include <memory>
using namespace std;
using namespace std::tr1;

struct FrameBuffer
{
	byte *pBuffer;
	int	 nLength;
	FrameBuffer(byte *pData,int nLen)
		:pBuffer(pData)
		, nLength(nLen)
	{
		pBuffer = new byte[nLen];
		memcpy(pBuffer, pData, nLen);
		nLength = nLen;
	}
	~FrameBuffer()
	{
		if (pBuffer)
		{
			delete[]pBuffer;
			pBuffer = nullptr;
			nLength = 0;
		}
	}
};
typedef shared_ptr<FrameBuffer> FrameBufferPtr;
#define _MaxBuffSize	(512*1024)
class CStreamParser
{
	byte *m_pStreamBuffer = nullptr;
	int	m_nStreamBufferSize ;			// 流数据的缓冲区长度
	int	m_nStreamBufferLength;			// 流缓冲区
	const AVCodec *m_pCodec = nullptr;
	AVCodecParserContext *parser;
	AVCodecContext *m_pAvContext = NULL;
	AVPacket *pAvPacket = nullptr;
	int		m_nPaserOffset = 0;
	bool	m_bFrameCacheFulled = false;
	int		m_nFrameIndex = 0;
public:
	CRITICAL_SECTION	m_csBuffer;
	list<FrameBufferPtr> m_listStream;
	CStreamParser()
	{
		ZeroMemory(this, offsetof(CStreamParser, m_csBuffer));
		m_nStreamBufferSize = 128 * 1024;
		m_pStreamBuffer = new byte[m_nStreamBufferSize];
		ZeroMemory(m_pStreamBuffer, m_nStreamBufferSize);
		InitializeCriticalSection(&m_csBuffer);	
		
	}
	int InitStreamParser(AVCodecID  nCodec = AV_CODEC_ID_H264)
	{
		pAvPacket = av_packet_alloc();
		if (!pAvPacket)
		{
			return IPC_Error_InsufficentMemory;
		}
		m_pCodec = avcodec_find_decoder(nCodec);
		if (!m_pCodec)
			return IPC_Error_UnsupportedCodec;

		parser = av_parser_init(AV_CODEC_ID_H264);
		if (!parser)
			return IPC_Error_ParserNotFound;				///< 找不到匹配的解析器

		m_pAvContext = avcodec_alloc_context3(m_pCodec);
		if (!m_pAvContext)
			return IPC_Error_AllocateCodecContextFaled;		///< 分配编码上下文失败

		if (avcodec_open2(m_pAvContext, m_pCodec, NULL) < 0)
			return IPC_Error_OpenCodecFailed;
		return IPC_Succeed;
	}
	void SetFrameCahceFulled(bool bFulled = true)
	{
		m_bFrameCacheFulled = bFulled;
	}
	int BufferLength()
	{
		CAutoLock lock(&m_csBuffer);
		return m_nStreamBufferLength;
	}
	int InputStream(byte *pData, int nLength)
	{
		CAutoLock lock(&m_csBuffer);
		if (m_bFrameCacheFulled ||m_nStreamBufferSize > _MaxBuffSize  )
			return IPC_Error_FrameCacheIsFulled;
		
		MemMerge(&m_pStreamBuffer, m_nStreamBufferLength, m_nStreamBufferSize, pData, nLength);
		return 0;
	}
	static void FreeAvPakcet(AVPacket *pAvPacket)
	{
		av_packet_free(&pAvPacket);
	}

	void OutputFile(UCHAR *szBuffer, UINT nLength, CHAR *szFileName)
	{
		if (szFileName == NULL || strlen(szFileName) == 0)
		{
			return;
		}
		else
		{
			HANDLE hLogFile = NULL;
			DWORD dwWritten = 0;

			__try
			{
				hLogFile = CreateFileA(szFileName,
					GENERIC_WRITE | GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_ALWAYS,
					FILE_ATTRIBUTE_ARCHIVE,
					NULL);
				if (hLogFile == NULL)
					return;
				WriteFile(hLogFile, szBuffer, nLength, &dwWritten, NULL);
				FlushFileBuffers(hLogFile);
				CloseHandle(hLogFile);
			}
			__except (1)
			{
				if (hLogFile != NULL)
					CloseHandle(hLogFile);
			}
		}
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
	int ParserFrame(byte *pData, int nLength, list<FrameBufferPtr> &listFrame)
	{
		while (nLength > 0)
		{			
			int ret = av_parser_parse2(parser, m_pAvContext, &pAvPacket->data, &pAvPacket->size, pData, nLength, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
			if (ret < 0)
			{
				TraceMsgA("%s Error while parsing.\n", __FUNCTION__);
				return listFrame.size();
			}
			if (pAvPacket->size > 0)
				listFrame.push_back(make_shared<FrameBuffer>(pAvPacket->data, pAvPacket->size));
			pData += ret;
			nLength -= ret;
		}
		return listFrame.size();
	}
	int ParserFrame(list<FrameBufferPtr> &listFrame)
	{
		CAutoLock lock(&m_csBuffer);
		if (m_nStreamBufferLength == 0)
			return m_nStreamBufferLength;
		byte *pData = m_pStreamBuffer;
		AVPacket *pAvPacket = av_packet_alloc();
		shared_ptr<AVPacket> AvPacketPtr(pAvPacket, FreeAvPakcet);
		while (m_nStreamBufferLength > 0)
		{
			int ret = av_parser_parse2(parser, m_pAvContext, &pAvPacket->data, &pAvPacket->size, pData, m_nStreamBufferLength, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
			if (ret < 0)
			{
				TraceMsgA("%s Error while parsing.\n", __FUNCTION__);
				return listFrame.size();
			}
			if (pAvPacket->size > 0)
				listFrame.push_back(make_shared<FrameBuffer>(pAvPacket->data, pAvPacket->size));
			pData += ret;
			m_nStreamBufferLength -= ret;
		}
		return listFrame.size();
	}
	~CStreamParser()
	{
		if (pAvPacket)
		{
			av_packet_free(&pAvPacket);
			pAvPacket = nullptr;
		}

		if (parser)
		{
			av_parser_close(parser);
			parser = nullptr;
		}
		if (m_pAvContext)
		{
			avcodec_free_context(&m_pAvContext);
			m_pAvContext = nullptr;
		}
		if (m_pStreamBuffer)
		{
			delete[]m_pStreamBuffer;
			m_pStreamBuffer = 0;
			m_nStreamBufferLength = 0;
		}
		DeleteCriticalSection(&m_csBuffer);
	}
};



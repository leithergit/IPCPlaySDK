
#include "Common.h"
#include "Media.h"
#include "IPCPlaySDK.h"
#include "./DxSurface/DxSurface.h"
#include "AutoLock.h"
#include "./VideoDecoder/VideoDecoder.h"

class CStreamParser
{
	byte *m_pStreamBuffer = nullptr;
	int	m_nStreamBufferSize ;		// 流数据的缓冲区长度
	int	m_nStreamBufferLength;					// 流缓冲区
	const AVCodec *m_pCodec = nullptr;
	AVCodecParserContext *parser;
	AVCodecContext *m_pAvContext = NULL;
	CRITICAL_SECTION	m_csBuffer;
	int		m_nPaserOffset = 0;
public:
	CStreamParser()
	{
		ZeroMemory(this, sizeof(CStreamParser));
		m_nStreamBufferSize = 128 * 1024;
		m_pStreamBuffer = new byte[m_nStreamBufferSize];
		ZeroMemory(m_pStreamBuffer, m_nStreamBufferSize);
		InitializeCriticalSection(&m_csBuffer);		
	}
	int InitStreamParser(AVCodecID  nCodec = AV_CODEC_ID_H264)
	{
		/* find the MPEG-1 video decoder */
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
	int BufferLength()
	{
		CAutoLock lock(&m_csBuffer);
		return m_nStreamBufferLength;
	}
	int InputStream(byte *pData, int nLength)
	{
		CAutoLock lock(&m_csBuffer);
		MemMerge(&m_pStreamBuffer, m_nStreamBufferLength, m_nStreamBufferSize, pData, nLength);
		return 0;
	}

	bool ParserFrame(AVPacket *pAvPacket)
	{
		CAutoLock lock(&m_csBuffer);
		if (m_nStreamBufferLength == 0)
			return false;
		int ret = av_parser_parse2(parser, m_pAvContext, &pAvPacket->data, &pAvPacket->size, m_pStreamBuffer, m_nStreamBufferLength, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		if (ret < 0)
		{
			TraceMsgA("%s Error while parsing.\n", __FUNCTION__);
			return false;
		}
		memmove(m_pStreamBuffer, &m_pStreamBuffer[ret], m_nStreamBufferLength - ret);
		m_nStreamBufferLength -= ret;
//		m_nPaserOffset += ret;
// 		if (m_nStreamBufferLength == m_nPaserOffset)
// 		{
// 			m_nPaserOffset = 0;
// 			m_nStreamBufferLength = 0;
// 		}
		return true;
	}
	~CStreamParser()
	{
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
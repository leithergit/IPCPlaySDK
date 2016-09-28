// DhStreamParser.cpp: implementation of the DhStreamParser class.
//
//////////////////////////////////////////////////////////////////////
#include "DhStreamParser.h"
#include "StreamParser.h"
#include "newstream.h"
#include "psstream.h"
#include "dhstdstream.h"
#include "svacstream.h"
#include "avistream.h"
#include "xmstream.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#pragma warning(disable:4305)
#pragma warning(disable:4309)

DhStreamParser::DhStreamParser(int nStreamType, int nFlag)
{
	m_nStreamType = nStreamType ;

	if (m_nStreamType == DH_STREAM_NEW)
	{
		m_streamParser = new NewStream(m_buffer) ;
	}
	else
	{
		m_nStreamType = DH_FRAME_TYPE_UNKNOWN ;
		m_streamParser = NULL ;
	}
}

DhStreamParser::~DhStreamParser()
{
	if (m_streamParser)
	{
		delete m_streamParser ;
		m_streamParser = NULL ;
	}
}

int DhStreamParser::InputData(unsigned char *pData, unsigned long nDataLength)
{
	if (m_nStreamType == DH_FRAME_TYPE_UNKNOWN 
		|| m_nStreamType == DH_STREAM_AUDIO)
	{
		int iRet = AutoScanStream(pData, nDataLength);
		if ( iRet< 0)
		{
			if (m_nStreamType == DH_STREAM_AUDIO && m_streamParser)//纯音频数据
			{
				return m_streamParser->ParseData(pData, nDataLength) ;
			}
			else//没找到
			{
				return -1 ;
			}
		}
	}

	return m_streamParser->ParseData(pData, nDataLength) ;
}

DH_FRAME_INFO *DhStreamParser::GetNextFrame()
{	
	if (m_nStreamType == DH_FRAME_TYPE_UNKNOWN)
	{
		return NULL ;
	}

	return m_streamParser->GetNextFrame() ; 
}

DH_FRAME_INFO *DhStreamParser::GetNextKeyFrame()
{
	if (m_nStreamType == DH_FRAME_TYPE_UNKNOWN)
	{
		return NULL ;
	}

	return m_streamParser->GetNextKeyFrame() ;
}

int DhStreamParser::Reset(int nLevel, int streamtype)
{
	if (m_nStreamType == DH_FRAME_TYPE_UNKNOWN)
	{
		if (streamtype == DH_STREAM_NEW && nLevel == STREAMPARSER_RESET_REFIND)
		{
			m_nStreamType = streamtype;
			m_streamParser = new NewStream(m_buffer) ;
		}

		return -1 ;
	}

	if (nLevel == STREAMPARSER_RESET_REFIND)
	{
		if (streamtype == DH_STREAM_NEW)
		{
			if (m_streamParser)
			{
				delete m_streamParser;
			}

			m_nStreamType = DH_STREAM_NEW;
			m_streamParser = new NewStream(m_buffer) ;

			return 0;
		}

		m_nStreamType = DH_FRAME_TYPE_UNKNOWN ;

		return 0 ;
	}

	return m_streamParser->Reset(nLevel) ;
}

int DhStreamParser::GetStreamType()
{
	return m_nStreamType ;
}

int DhStreamParser::AutoScanStream(unsigned char *pData, unsigned long nDataLength)
{
	static const unsigned long MAXSCANLEN = 51200 ;
	
	int NEWStreamCounter = 0;
	int XMStreamCounter = 0;
	int AUDIOStreamCounter = 0;
	int AVIStreamCounter = 0;
	int PSStreamCounter = 0;	
	int DHSTDStreamCounter = 0;
	int SVACStreamCounter = 0;
	unsigned int Code = 0xFFFFFFFF;
	unsigned char *pScanBuf = pData;
	unsigned long DataRest = nDataLength ;
	unsigned long asfpacketlen = 0;
	unsigned int videoStreamID = 1;

	while (DataRest--)
	{
		Code = (Code << 8) | *pScanBuf++;
	
		if (Code == 0x01EA)
		{
			if (DataRest < 20)
			{
				continue;
			}
			SVACStreamCounter = 1 ;

			if (nDataLength - DataRest > MAXSCANLEN)
			{
				break ;
			}
		}
		else if (Code == 0x01ED || Code == 0x01EC)
		{
			if (DataRest < 20)
			{
				continue;
			}

			int encodeType = pScanBuf[0];
			if (encodeType < 1 || encodeType > 5)//目前只有1-4这四种编码格式
			{
				continue;
			}

			int width  = (pScanBuf[4] | pScanBuf[5] << 8);
			int height = (pScanBuf[6] | pScanBuf[7] << 8);
			int rate   = pScanBuf[2];

			if (width < 120 || width > 4096)
			{
				continue;
			}

			if (height < 120 || height > 4096)
			{
				continue;
			}

			if (rate > 100 || rate < 1)
			{
				continue;
			}

			SVACStreamCounter = 1 ;

			if (nDataLength - DataRest > MAXSCANLEN)
			{
				break ;
			}
		}

		if (Code == 0x01FC)
		{
			if (DataRest < 12)
			{
				continue;
			}
			
			int width = pScanBuf[2] * 8;
			int height = pScanBuf[3] * 8;
			int rate = pScanBuf[1] & 0x1F;
			
			if (width < 120 || width > 1920)
			{
				continue;
			}
			if (height < 120 || height > 1920)
			{
				continue;
			}
			
			if (rate > 30 || rate < 1)
			{
				continue;
			}
			
			XMStreamCounter = 1 ;
			
			if (nDataLength - DataRest > MAXSCANLEN)
			{
				break ;
			}
		}
		else if (Code == 0x01FD || Code == 0x01FE)
		{
			if (DataRest < 12)
			{
				continue;
			}

			int width = pScanBuf[2] * 8;
			int height = pScanBuf[3] * 8;
			int rate = pScanBuf[1] & 0x1F;

			if (width < 120 || width > 1920)
			{
				continue;
			}
			if (height < 120 || height > 1920)
			{
				continue;
			}

			if (rate > 100 || rate < 1)
			{
				continue;
			}

		 	NEWStreamCounter = 1 ;
			
			if (nDataLength - DataRest > MAXSCANLEN)
			{
				break ;
			}
		}
		else if (Code == 0x01F0)
		{
			AUDIOStreamCounter = 1 ;

			if (nDataLength - DataRest > MAXSCANLEN)
			{
				break ;
			}
		}
		else if (Code == 0x52494646)//RIFF
		{
			if (DataRest > 8)
			{
				unsigned int rifftype = pScanBuf[4]<<24|pScanBuf[5]<<16|pScanBuf[6]<<8|pScanBuf[7];
				if (rifftype==0x41564920) {
					AVIStreamCounter = 1;
					break;
				}
			}
		}
		else if (Code == 0x01BA)
		{
			PSStreamCounter = 1;
			if (nDataLength - DataRest > MAXSCANLEN)
			{
				break ;
			}
		}
		else if (Code == 0x44484156)
		{
			if (DataRest < 20)
			{
				continue;
			}

			unsigned char crc = 0 ;	
			for (int i = 0; i < 19; i++)
			{
				crc += pScanBuf[i];
			}
			crc += 0x44 ;
			crc += 0x48 ;
			crc += 0x41 ;
			crc += 0x56 ;

			if (crc != pScanBuf[19])
			{
				continue;
			}

			DHSTDStreamCounter = 1;
			if (nDataLength - DataRest > MAXSCANLEN)
			{
				break ;
			}
		}
	}

	if (AVIStreamCounter || NEWStreamCounter || XMStreamCounter
		|| PSStreamCounter || DHSTDStreamCounter || SVACStreamCounter)
	{
		if (m_streamParser)
		{
			delete m_streamParser;
			m_streamParser = NULL;
		}
	}


	if (AVIStreamCounter > 0)
	{
		DH_FRAME_INFO tmpAviInfo;
		memset(&tmpAviInfo, 0, sizeof(tmpAviInfo));

		int auds = 0;

		while (DataRest--)
		{
			Code = (Code << 8) | *pScanBuf++;

			if (Code == 0x61766968)
			{
				pScanBuf += 4;
				int resultion =*(int*)pScanBuf;
				tmpAviInfo.nFrameRate = 1000000/resultion;
				pScanBuf+=32;
				tmpAviInfo.nWidth=*(int*)pScanBuf;
				tmpAviInfo.nHeight=*(int*)(pScanBuf+4);
			}
			else if (Code == 0x61756473)
			{
				auds = 1;
			}
			else if (Code == 0x73747266)
			{
				if (auds == 1 && DataRest > 20)
				{
					tmpAviInfo.nSamplesPerSecond = *((int*)(pScanBuf+8));
					tmpAviInfo.nBitsPerSample = (pScanBuf[16] | (pScanBuf[17] << 8))  * 8;
					break;
				}
			}
		}

		if (tmpAviInfo.nBitsPerSample != 8 && tmpAviInfo.nBitsPerSample != 16)
		{
			tmpAviInfo.nBitsPerSample = 8;
		}

		if (tmpAviInfo.nSamplesPerSecond < 4000 || tmpAviInfo.nSamplesPerSecond > 48000)
		{
			tmpAviInfo.nSamplesPerSecond = 8000;
		}
		m_nStreamType = DH_STREAM_AVI;
		m_streamParser = new AVIStream(m_buffer, &tmpAviInfo);
		return 0;
	}
	else if (DHSTDStreamCounter > 0)
	{
		m_nStreamType = DH_STREAM_DHSTD;
		m_streamParser = new DhStdStream(m_buffer);
		return 0;
	}
	else if (NEWStreamCounter > 0)
	{
		m_nStreamType = DH_STREAM_NEW ;
		m_streamParser = new NewStream(m_buffer) ;
		return 0 ;
	}
	else if (XMStreamCounter > 0)
	{
		m_nStreamType = DH_STREAM_XM ;
		m_streamParser = new XMStream(m_buffer) ;
		return 0 ;
	}
	else if (SVACStreamCounter > 0)
	{
		m_nStreamType = DH_STREAM_SVAC;
		m_streamParser = new SvacStream(m_buffer);
		return 0;
	}	
	else if (PSStreamCounter > 0)
	{
		m_nStreamType = DH_STREAM_PS ;
		m_streamParser = new PSStream(m_buffer) ;
		return 0 ;
	}
	else if (AUDIOStreamCounter > 0)
	{
		if (m_nStreamType != DH_STREAM_AUDIO)
		{
			if (m_streamParser)
			{
				delete m_streamParser;
				m_streamParser = NULL;
			}	
			m_nStreamType = DH_STREAM_AUDIO ;
			m_streamParser = new NewStream(m_buffer) ;//纯音频流用新码流来解
		}
		
		return 0 ;
	}

	return -1 ;
}

//chenf20090511-s
int DhStreamParser::GetFrameDataListSize()
{
	//return m_streamParser->GetDataListSize();
	return m_streamParser->m_FrameInfoList.GetDataListSize();
}
//chenf20090511-e

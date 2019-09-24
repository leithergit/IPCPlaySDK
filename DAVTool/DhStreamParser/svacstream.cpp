#include <time.h>
#include "SvacStream.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction

SvacStream::SvacStream(unsigned char* rawBuf):StreamParser(rawBuf)
{
	m_extendLen = 0;
}

SvacStream::~SvacStream()
{
}

inline
bool SvacStream::CheckSign(const unsigned int& Code)
{
	if (Code == 0x01ED || Code == 0x01EC || Code == 0x01EA)
	{
		return true ;
	}

	return false ;
}

inline
bool SvacStream::ParseOneFrame()
{
	if (rest < 20)
	{
		return false;
	}

	int extNum =  m_bufptr[14];
	unsigned char* extptr = m_bufptr + 16;
	m_extendLen = 0;
	while (extNum > 0)
	{
		int exttype = extptr[0] | extptr[1] << 8;
		int extlen  = 4 + extptr[2] | extptr[3] << 8;//2字节类型，2字节长度
		m_extendLen += extlen;

		if (rest < 20 + m_extendLen)
		{
			return false;
		}
		extptr += extlen;
		extNum--;
	}

	m_FrameInfo = m_FrameInfoList.GetFreeNote() ;
	switch (m_bufptr[0])
	{
	case 1:
		m_FrameInfo->nEncodeType = DH_ENCODE_VIDEO_MPEG4 ;
		break;
	case 2:
		m_FrameInfo->nEncodeType = DH_ENCODE_VIDEO_H264 ;
		break;
	case 3:
		m_FrameInfo->nEncodeType = DH_ENCODE_VIDEO_JPEG;
		break;
	case 4:
		m_FrameInfo->nEncodeType = DH_ENCODE_VIDEO_SVAC;
		break;
	case 5:
		m_FrameInfo->nEncodeType = DH_ENCODE_VIDEO_SVAC_NEW;
		break;
	default:
		m_FrameInfo->nEncodeType = DH_ENCODE_VIDEO_H264 ;
		break;
	}

	m_FrameInfo->nFrameRate  = m_bufptr[2] &0x1F;
	unsigned long tmpFrameSeq = m_bufptr[3] ;	
	m_FrameInfo->nWidth = m_bufptr[4] | m_bufptr[5]<<8 ;
	m_FrameInfo->nHeight = m_bufptr[6] | m_bufptr[7]<<8 ;

	unsigned long dt = m_bufptr[8] | m_bufptr[9] <<8 | m_bufptr[10] << 16 | m_bufptr[11] << 24;
	m_FrameInfo->nSecond = dt & 0x3f;
	m_FrameInfo->nMinute = (dt >> 6) & 0x3f;
	m_FrameInfo->nHour = (dt >> 12) & 0x1f;
	m_FrameInfo->nDay = (dt >> 17) & 0x1f;
	m_FrameInfo->nMonth = (dt >> 22) & 0xf;
	m_FrameInfo->nYear = 2000 + (dt >> 26);

	tm tmp_time ;
	tmp_time.tm_hour = m_FrameInfo->nHour /*- 1*/ ;
	tmp_time.tm_min  = m_FrameInfo->nMinute /*- 1*/ ;
	tmp_time.tm_sec  = m_FrameInfo->nSecond /*- 1*/ ;
	tmp_time.tm_mday = m_FrameInfo->nDay  /*+ 1*/;
	tmp_time.tm_mon  = m_FrameInfo->nMonth - 1 ;
	tmp_time.tm_year = m_FrameInfo->nYear - 1900 ;
	m_FrameInfo->nTimeStamp = mktime(&tmp_time) ;

	unsigned char* frameLenPtr = m_bufptr + m_extendLen;
	m_FrameInfo->nFrameLength = m_frameLen = 
		frameLenPtr[19]<<24|frameLenPtr[18]<<16|frameLenPtr[17]<<8|frameLenPtr[16] ;		
	
	if (m_code == 0x01ED)
	{
		m_FrameInfo->nType = DH_FRAME_TYPE_VIDEO ;
		m_FrameInfo->nSubType = DH_FRAME_TYPE_VIDEO_I_FRAME ;
		m_preShSeq = tmpFrameSeq;
	}
	else if (m_code == 0x01EC)
	{
		m_FrameInfo->nType = DH_FRAME_TYPE_VIDEO ;
		m_FrameInfo->nSubType = DH_FRAME_TYPE_VIDEO_P_FRAME ;

		if (m_preShSeq == 0)
		{
			m_preShSeq = tmpFrameSeq;
		}

		if (tmpFrameSeq > m_preShSeq+1  || tmpFrameSeq < m_preShSeq)
		{
			if (tmpFrameSeq == 0 && m_preShSeq == 255)
			{
			}
			else
			{
				m_FrameInfo->nFrameLength = m_frameLen = (m_frameLen-1);
			}
		}

		m_preShSeq = tmpFrameSeq;
	}
	else if(m_code == 0x01EA)
	{
		m_FrameInfo->nType = DH_FRAME_TYPE_AUDIO ;				
		m_FrameInfo->nEncodeType = m_bufptr[4] ;
		m_FrameInfo->nChannels = (m_bufptr[5] >= 2) ? 2 : 1;//只分双声道和单声道，没有左右声道之分
		AudioInfoOpr(m_FrameInfo, m_bufptr[6]) ;
		m_FrameInfo->nBitsPerSample = (m_bufptr[7] == 0) ? 8 : 16;
	}

	m_FrameInfo->pHeader = m_bufptr - 4 ;			
	m_FrameInfo->pContent = m_bufptr + 20 + m_extendLen ;
	m_FrameInfo->nLength  = m_FrameInfo->nFrameLength + 24 + m_extendLen ;

	rest -= (m_extendLen + 20) ;
	m_bufptr += (m_extendLen + 20) ;

	return true ;
}

inline
bool SvacStream::CheckIfFrameValid()
{
	static const unsigned int IFRAME_D_CODE = 0x000001ED ;
	static const unsigned int PFRAME_C_CODE = 0x000001EC ;
	static const unsigned int PFRAME_A_CODE = 0x000001EA ;

	int i = 4 ;
	m_code = 0 ;

	while (rest >0 && i--)
	{
		m_code = m_code << 8 | *m_bufptr++ ;
		rest-- ;
		if (m_code != (IFRAME_D_CODE >> (i*8)) && 
			m_code != (PFRAME_C_CODE >> (i*8)) && 
			m_code != (PFRAME_A_CODE >> (i*8)) )
		{
			return false ;
		}
	}

	return true ;
}

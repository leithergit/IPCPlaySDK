#include "oldstream.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

OldStream::OldStream(unsigned char* rawBuf):StreamParser(rawBuf)
{
}

OldStream::~OldStream()
{
}

bool OldStream::CheckSign(const unsigned int& Code)
{
	if (Code == 0x44485054 || Code == 0x01F0)
	{
		return true ;
	}

	return false ;
}

bool OldStream::ParseOneFrame()
{
	if (rest < 4)
	{
		return false ;
	}

	m_FrameInfo = m_FrameInfoList.GetFreeNote() ;

	if (m_code == 0x44485054)
	{   //ÊÓÆµÖ¡
		m_FrameInfo->nType = DH_FRAME_TYPE_VIDEO ;
		m_FrameInfo->nEncodeType = DH_ENCODE_VIDEO_MPEG4;
		m_FrameInfo->nFrameLength = m_frameLen = 
			m_bufptr[3]<<24|m_bufptr[2]<<16|m_bufptr[1]<<8|m_bufptr[0] ;			
		m_FrameInfo->pHeader = m_bufptr - 4 ;
		m_FrameInfo->pContent = m_bufptr + 4 ;
		m_FrameInfo->nLength  = m_FrameInfo->nFrameLength + 8 ;
		rest -= 4 ;
		m_bufptr += 4 ;
	}
	else if (m_code == 0x01F0)
	{   //ÒôÆµÖ¡
		m_FrameInfo->nType = DH_FRAME_TYPE_AUDIO ;
		m_FrameInfo->nEncodeType = m_bufptr[0] ;		
		m_FrameInfo->nChannels = 1;
		AudioInfoOpr(m_FrameInfo, m_bufptr[1]) ;
		
		m_FrameInfo->nFrameLength = m_frameLen = 
			m_bufptr[3]<<8|m_bufptr[2] ;			
		m_FrameInfo->pHeader = m_bufptr - 4 ;
		m_FrameInfo->pContent = m_bufptr + 4 ;
		m_FrameInfo->nLength  = m_FrameInfo->nFrameLength + 8 ;
		rest -= 4 ;
		m_bufptr += 4 ;
	}

	return true ;
}

bool OldStream::CheckIfFrameValid()
{
	static const unsigned int DHPTCODE = 0x44485054 ;
	static const unsigned int AUDIOCODE = 0x000001F0 ;
	int i = 4 ;
	m_code = 0 ;

	while (rest >0 && i--)
	{
		m_code = m_code << 8 | *m_bufptr++ ;
		rest-- ;
		if (m_code != (DHPTCODE >> (i*8)) && m_code != (AUDIOCODE >> (i*8)))
		{
			return false ;
		}
	}
	
	unsigned char *pBuf = m_FrameInfo->pContent ;
	
	if (m_FrameInfo->nType == DH_FRAME_TYPE_VIDEO)
	{
		unsigned int code = pBuf[0] << 24 | pBuf[1] << 16 | pBuf[2] << 8 | pBuf[3];

		if (code == 0x00000100)//IÖ¡
		{
			code = 0xffffffff ;
			pBuf += 4;
			
			while (code != 0x01B6)
			{
				code = code << 8 | *pBuf++;

				if (code == 0x0120)
				{
					int t = pBuf[1];

					if (t == 4)
					{
						m_FrameInfo->nWidth = (pBuf[5]<<2)|((pBuf[6]&0xc0)>>6);
						m_FrameInfo->nHeight = ((pBuf[6]&0x0f)<<8)|pBuf[7];
						pBuf += 8 ;
					}
					else if (t == 6)
					{
						m_FrameInfo->nWidth = (pBuf[6]<<3);
						m_FrameInfo->nHeight = (pBuf[8]<<1);
						pBuf += 8 ;
					}
					else 
					{
						m_FrameInfo->nWidth = 352;
						m_FrameInfo->nHeight = 288;
					}
				}//end of if (code == 0x0120)
				else if (code == 0x01B3)
				{
					m_FrameInfo->nHour	= pBuf[0]>>3;
					m_FrameInfo->nMinute	= (pBuf[0]&0x07)<<3 | pBuf[1]>>5;
					m_FrameInfo->nSecond	= (pBuf[1]&0x0f)<<2 | pBuf[2]>>6;
					m_FrameInfo->nTimeStamp = (m_FrameInfo->nHour*3600+
						m_FrameInfo->nMinute*60+m_FrameInfo->nSecond)/**1000*/;
					pBuf += 2 ;
				}
				else if (code == 0x01B2)
				{
					m_FrameInfo->nFrameRate = pBuf[6];
					if (m_FrameInfo->nFrameRate > 150)
					{
						if (m_FrameInfo->nFrameRate == 0xFF)
						{
							m_FrameInfo->nFrameRate = 1;
						}
						else
						{
							m_FrameInfo->nFrameRate = 25;
						}
					}//end of if (m_FrameInfo->nFrameRate > 150 || m_FrameInfo->nFrameRate < 0)		
				}//end of else if (code == 0x01B2)
			}//end of while (code != 0x01B6)

			m_FrameInfo->nSubType = DH_FRAME_TYPE_VIDEO_I_FRAME;			
		}
		else if (code == 0x01B6)
		{			
			m_FrameInfo->nSubType = DH_FRAME_TYPE_VIDEO_P_FRAME;
		}
		else
		{
			m_FrameInfo->nFrameLength = 0 ;
		}
	}//end of if (m_FrameInfo->nType == DH_FRAME_TYPE_VIDEO)

	return true ;
}
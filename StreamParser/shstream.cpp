#include "shstream.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

ShStream::ShStream(unsigned char* rawBuf):StreamParser(NULL)
{
	m_rawBuf = m_shbuffer ;
	m_rwstream = new RwStream(rawBuf, MPEG_SH2) ;
	m_preShSeq = 0 ;
}

ShStream::~ShStream()
{
	if (m_rwstream)
	{
		delete m_rwstream ;
		m_rwstream = 0 ;
	}
}

bool ShStream::CheckSign(const unsigned int& Code)
{
	if (Code == 0x01F2 || Code == 0x01F0)
	{
		return true ;
	}

	return false ;
}

bool ShStream::ParseOneFrame()
{
	if ((m_code == 0x01F2) && (rest >= 12))
	{
		m_FrameInfo = m_FrameInfoList.GetFreeNote() ;

		m_FrameInfo->nType = DH_FRAME_TYPE_VIDEO ;		
		m_FrameInfo->nEncodeType = DH_ENCODE_VIDEO_MPEG4 ;	

		m_FrameInfo->nFrameRate  = m_bufptr[1] >> 3;
		int size = m_bufptr[2] | m_bufptr[3] << 8 ;
		m_FrameInfo->nWidth = ((size>>3)&0x3f)*16;
        m_FrameInfo->nHeight = ((size>>9)&0x7f)*16;
		
		long dt = m_bufptr[4] | m_bufptr[5] << 8 | m_bufptr[6] << 16 | m_bufptr[7] << 24;
		m_FrameInfo->nParam1 = m_bufptr[8] | m_bufptr[9] << 8 | m_bufptr[10] << 16 | m_bufptr[11] << 24;

		struct tm *newtime = gmtime((const time_t *)&dt);
	
		if (newtime != 0)
		{
			m_FrameInfo->nTimeStamp = mktime(newtime) ;
			m_FrameInfo->nSecond	= newtime->tm_sec;
			m_FrameInfo->nMinute	= newtime->tm_min;
			m_FrameInfo->nHour		  = newtime->tm_hour;
			m_FrameInfo->nDay	  	   = newtime->tm_mday;
			m_FrameInfo->nMonth		= newtime->tm_mon + 1;
			m_FrameInfo->nYear		   = newtime->tm_year + 1900;
		}

		m_FrameInfo->nFrameLength = m_frameLen = 1024 ;					
		m_FrameInfo->pHeader = m_bufptr - 4 ;			
		m_FrameInfo->pContent = m_bufptr + 12 ;
		m_FrameInfo->nLength  = 1040 ;
		rest -= 12 ;
		m_bufptr += 12 ;
		return true ;
	}
	else if ((m_code == 0x01F0) && (rest >= 12))
	{		
		m_FrameInfo = m_FrameInfoList.GetFreeNote() ;

		m_FrameInfo->nType = DH_FRAME_TYPE_AUDIO ;	
		m_FrameInfo->nEncodeType = m_bufptr[0];// ENCODE
		m_FrameInfo->nChannels = 1;
		
		AudioInfoOpr(m_FrameInfo, m_bufptr[1]) ;

		m_FrameInfo->nFrameLength = m_frameLen = m_bufptr[3] <<8 | m_bufptr[2] ;		
		m_FrameInfo->pHeader = m_bufptr - 4 ;
		m_FrameInfo->pContent = m_bufptr + 12 ;
		m_FrameInfo->nLength  = m_frameLen + 16 ;
		rest -= 12 ;
		m_bufptr += 12 ;	
		return true ;
	}

	return false ;
}

bool ShStream::CheckIfFrameValid()
{
	static const unsigned int VIDEOCODE = 0x000001F2 ;
	static const unsigned int AUDIOCODE = 0x000001F0 ;
	int i = 4 ;
	m_code = 0 ;

	while (rest >0 && i--)
	{
		m_code = m_code << 8 | *m_bufptr++ ;
		rest-- ;
		if (m_code != (VIDEOCODE >> (i*8)) && m_code != (AUDIOCODE >> (i*8)))
		{
			return false ;
		}
	}
	
	return true ;
}

int ShStream::ParseData(unsigned char *data, int datalen) 
{
	StreamParser::ParseData(data, datalen) ;

	while(m_ShFrameInfo = StreamParser::GetNextFrame()) 
	{
		if (m_ShFrameInfo->nType == DH_FRAME_TYPE_VIDEO)
		{
			if (m_preShSeq == 0)
			{
				m_rwstream->ReSet(0) ;
				m_preShSeq = m_ShFrameInfo->nParam1 ;
			}
				
			if (m_ShFrameInfo->nParam1 > m_preShSeq+1  || m_ShFrameInfo->nParam1 < m_preShSeq)
			{
				m_preShSeq = 0 ;
			 	m_rwstream->ReSet(0) ;
				continue ;
			}
		
			m_preShSeq = m_ShFrameInfo->nParam1 ;	

			if (m_ShFrameInfo->nFrameLength == 0)
			{
				m_preShSeq = 0 ;
			 	m_rwstream->ReSet(0) ;
				continue ;
			}
			
			m_rwstream->GetShFrameInfo(m_ShFrameInfo) ;
		 	m_rwstream->ParseData(m_ShFrameInfo->pContent, m_ShFrameInfo->nFrameLength) ;
		}

		else if (m_ShFrameInfo->nType == DH_FRAME_TYPE_AUDIO)
		{
			if (m_ShFrameInfo->nFrameLength == 0)
			{
				m_rwstream->ReSet(0) ;
				continue ;		
			}

			DH_FRAME_INFO* tmp_frame = m_rwstream->m_FrameInfoList.GetFreeNote();
			memcpy(tmp_frame, m_ShFrameInfo, sizeof(DH_FRAME_INFO)) ;
	 		m_rwstream->AddFrameInfo(tmp_frame) ;
		}
	}

	return 0 ;
}

DH_FRAME_INFO *ShStream::GetNextFrame()
{
	DH_FRAME_INFO* tmp_shstream = m_rwstream->GetNextFrame() ;

	if (tmp_shstream == NULL)
	{
		m_rwstream->ReSet(1) ;
	}

	return tmp_shstream ;
}

DH_FRAME_INFO *ShStream::GetNextKeyFrame() 
{
	return m_rwstream->GetNextKeyFrame() ;
}

// newstream.cpp: implementation of the newstream class.
//
//////////////////////////////////////////////////////////////////////

#include <time.h>
#include "newstream.h"
#include "decSPS.h"
#include <Windows.h>
/*#include <windows.h>*/
//////////////////////////////////////////////////////////////////////
// Construction/Destruction

NewStream::NewStream(unsigned char* rawBuf):StreamParser(rawBuf)
{
}

NewStream::~NewStream()
{
}

inline
bool NewStream::CheckSign(const unsigned int& Code)
{
	if (Code == 0x01FD || Code == 0x01FC || Code == 0x01FE || Code == 0x01F0)
	{ 
		return true ;
	}

	return false ;
}

inline
bool NewStream::ParseOneFrame()
{
	static int frameNum = 0;

	if ((m_code == 0x01FD || m_code == 0x01FE) && (rest >= 12))
	{
		m_FrameInfo = m_FrameInfoList.GetFreeNote() ;
		//2010-1-16刘阳修改start
		m_FrameInfo->nSerial = m_bufptr[11];
		//2010-1-16刘阳修改end
		m_FrameInfo->nParam2 = m_bufptr[0]/*2*/ ;
		m_FrameInfo->nFrameRate  = m_bufptr[1] &0x1F;
		m_FrameInfo->nParam1 = m_bufptr[1]>>5;
		m_FrameInfo->nWidth = m_bufptr[2]*8 ;
		m_FrameInfo->nHeight = m_bufptr[3]*8 ;

		unsigned long dt = m_bufptr[4] | m_bufptr[5] <<8 | m_bufptr[6] << 16 | m_bufptr[7] << 24;
		m_FrameInfo->nSecond = dt & 0x3f;
		m_FrameInfo->nMinute = (dt >> 6) & 0x3f;
		m_FrameInfo->nHour = (dt >> 12) & 0x1f;
		m_FrameInfo->nDay = (dt >> 17) & 0x1f;
		m_FrameInfo->nMonth = (dt >> 22) & 0xf;
		m_FrameInfo->nYear = 2000 + (dt >> 26);

		//FILE* fp = fopen("d://fileInfo.txt", "ab");
		//fprintf(fp, "I Frame, num=%d, time=%d-%d-%d:%d-%d-%d\n", frameNum++, m_FrameInfo->nYear, m_FrameInfo->nMonth, m_FrameInfo->nDay, 
		//	m_FrameInfo->nHour, m_FrameInfo->nMinute, m_FrameInfo->nSecond);
		//fclose(fp);
		
		tm tmp_time ;
		tmp_time.tm_hour = m_FrameInfo->nHour ;
		tmp_time.tm_min  = m_FrameInfo->nMinute ;
		tmp_time.tm_sec  = m_FrameInfo->nSecond ;
		tmp_time.tm_mday = m_FrameInfo->nDay ;
		tmp_time.tm_mon  = m_FrameInfo->nMonth - 1 ;
		tmp_time.tm_year = m_FrameInfo->nYear - 1900 ;
		m_FrameInfo->nTimeStamp = mktime(&tmp_time) ;

		m_FrameInfo->nType = DH_FRAME_TYPE_VIDEO ;
		m_FrameInfo->nSubType = DH_FRAME_TYPE_VIDEO_I_FRAME ;

		if (m_code == 0x01FD)//if (m_code == 0x01FC)
		{
              if(3!=m_bufptr[0])
				m_FrameInfo->nEncodeType = DH_ENCODE_VIDEO_H264 ;
			  else 
				m_FrameInfo->nEncodeType = DH_ENCODE_VIDEO_JPEG ;
		}
		else if (m_code == 0x01FE)
 		{
 			m_FrameInfo->nEncodeType = DH_ENCODE_VIDEO_JPEG ;
 		}

		m_FrameInfo->nFrameLength = m_frameLen = 
			/*m_bufptr[11]<<24|*/m_bufptr[10]<<16|m_bufptr[9]<<8|m_bufptr[8] ;		
			
		m_preShSeq = m_bufptr[11] ;	

		m_FrameInfo->pHeader = m_bufptr - 4 ;			
		m_FrameInfo->pContent = m_bufptr + 12 ;
		m_FrameInfo->nLength  = m_FrameInfo->nFrameLength + 16 ;
		rest -= 12 ;
		m_bufptr += 12 ;

		//2010-1-16刘阳修改start
		m_frameNotCon = false;
		m_FrameInfo->nDiscardFrame = 0;

		//decsps(m_FrameInfo->pContent, m_FrameInfo->nLength, &width, &height);
		return true ;
	}
	else if ((m_code == 0x01FC || m_code == 0x01FA/* || m_code == 0x01FA*/) && (rest >= 4))//P帧和A帧 //else if ((m_code == 0x01FD/* || m_code == 0x01FA*/) && (rest >= 4))//P帧和A帧
	{
		//FILE* fp = fopen("d://fileInfo.txt", "ab");
		//fprintf(fp, "P Frame, num=%d\n", frameNum++);
		//fclose(fp);

		m_FrameInfo = m_FrameInfoList.GetFreeNote() ;
		m_FrameInfo->nType = DH_FRAME_TYPE_VIDEO ;
		m_FrameInfo->nSubType = DH_FRAME_TYPE_VIDEO_P_FRAME ;

		if (m_code == 0x01FC)//if (m_code == 0x01FD)
		{
			m_FrameInfo->nEncodeType = DH_ENCODE_VIDEO_H264 ;
		}
		else
		{
			m_FrameInfo->nEncodeType = DH_ENCODE_VIDEO_MPEG4 ;
		}

		unsigned long nByte1 = m_bufptr[2];
		unsigned long nByte2 = m_bufptr[1];
		unsigned long nByte3 = m_bufptr[0];

		m_FrameInfo->nFrameLength = m_frameLen = nByte1<<16 | nByte2<<8 | nByte3;
				
		int tmpframelen = m_FrameInfo->nFrameLength;

		if (m_bufptr[3] > m_preShSeq+1  || m_bufptr[3] <= m_preShSeq)
		{
			if (m_bufptr[3] == 1 && m_preShSeq == 255)//来自平台
			{
			}
			else if (m_preShSeq == 0)//来自直连
			{
				m_frameNotCon = false;
			}
			else
			{
				m_frameNotCon = true;
			}
		}
		
		m_FrameInfo->nSerial = m_bufptr[3];
	
		m_preShSeq = m_bufptr[3] ;	

		m_FrameInfo->pHeader = m_bufptr - 4 ;
		m_FrameInfo->pContent = m_bufptr + 4 ;
		m_FrameInfo->nLength  = m_FrameInfo->nFrameLength + 8 ;
		rest -= 4 ;
		m_bufptr += 4 ;
		
		if (m_preShSeq == 0)
		{
			m_FrameInfo->nDiscardFrame = 0;
		}
		else
		{
			if (m_frameNotCon)
				m_FrameInfo->nDiscardFrame = 1;
			else
				m_FrameInfo->nDiscardFrame = 0;
		}

		return true ;
	}
	else if ((m_code == 0x01F0) && (rest >= 4))//音频帧//((m_code == 0x01FA) && (rest >= 4))//音频帧
	{
		//static int audioframe = 0;
		//FILE* fp = fopen("d://fileInfo.txt", "ab");
		//fprintf(fp, "------Audio Frame, num=%d\n", audioframe++);
		//fclose(fp);

		m_FrameInfo = m_FrameInfoList.GetFreeNote() ;
		m_FrameInfo->nType = DH_FRAME_TYPE_AUDIO ;				
		m_FrameInfo->nEncodeType = m_bufptr[0] ;
		if (DH_ENCODE_AUDIO_AAC == m_FrameInfo->nEncodeType)
		{
			m_FrameInfo->nChannels = 2;
		}
		else
		{
			m_FrameInfo->nChannels = 1;	
		}
		
		AudioInfoOpr(m_FrameInfo, m_bufptr[1]) ;

		if (m_FrameInfo->nEncodeType == 7 && m_bufptr[1] == 4)
		{
			m_FrameInfo->nChannels = 2;
			m_FrameInfo->nBitsPerSample = 16;
			m_FrameInfo->nSamplesPerSecond = 16000;
		}
		m_FrameInfo->nFrameLength = m_frameLen = m_bufptr[3] <<8 | m_bufptr[2] ;		
		m_FrameInfo->pHeader = m_bufptr - 4 ;
		m_FrameInfo->pContent = m_bufptr + 4 ;
		m_FrameInfo->nLength  = m_frameLen + 8 ;
		rest -= 4 ;
		m_bufptr += 4 ;	

		return true ;
	}

	return false ;
}

inline
bool NewStream::CheckIfFrameValid()
{
	static const unsigned int IFRAME_D_CODE = 0x000001FD ;
	static const unsigned int IFRAME_E_CODE = 0x000001FE ;
	static const unsigned int PFRAME_C_CODE = 0x000001FC ;
	static const unsigned int AUDIOCODE = 0x000001F0 ;

	int i = 4 ;
	m_code = 0 ;

	while (rest >0 && i--)
	{
		m_code = m_code << 8 | *m_bufptr++ ;
		rest-- ;
		if (m_code != (IFRAME_D_CODE >> (i*8)) 
			&& m_code != (IFRAME_E_CODE >> (i*8))
			&& m_code != (PFRAME_C_CODE >> (i*8)) 
			&& m_code != (AUDIOCODE >> (i*8)))
		{
			return false ;
		}
	}	

	return true ;
}

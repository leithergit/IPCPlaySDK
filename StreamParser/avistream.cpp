#include "avistream.h"

AVIStream::AVIStream(unsigned char* rawBuf, DH_FRAME_INFO* AviInfo):StreamParser(rawBuf)
{
	m_width = AviInfo->nWidth;
	m_height = AviInfo->nHeight;
	m_rate = AviInfo->nFrameRate;
	m_bitsPerSample = AviInfo->nBitsPerSample;
	m_samplesPerSecond = AviInfo->nSamplesPerSecond;
}

AVIStream::~AVIStream()
{
}

inline
bool AVIStream::CheckSign(const unsigned int& Code)
{
	if (Code == 0x30306463 //00dc
	 || Code == 0x30317762 //01wb
	   )
	{
		return true;
	}
	
	return false ;
}

inline
bool AVIStream::ParseOneFrame()
{
	if ((m_code == 0x30306463) && (rest >= 12))
	{
		m_FrameInfo = m_FrameInfoList.GetFreeNote() ;
		m_FrameInfo->nFrameRate  = m_rate;
		m_FrameInfo->nWidth = m_width;
		m_FrameInfo->nHeight = m_height;

		m_FrameInfo->nType = DH_FRAME_TYPE_VIDEO ;
		m_FrameInfo->nSubType = DH_FRAME_TYPE_VIDEO_I_FRAME ;

	
		m_FrameInfo->nEncodeType = DH_ENCODE_VIDEO_H264 ;
	

		m_FrameInfo->nFrameLength = m_frameLen = m_bufptr[2]<<16|m_bufptr[1]<<8|m_bufptr[0] ;		
	
		m_FrameInfo->pHeader = m_bufptr - 4 ;			
		m_FrameInfo->pContent = m_bufptr + 4 ;
		m_FrameInfo->nLength  = m_FrameInfo->nFrameLength + 8 ;
		rest -= 4 ;
		m_bufptr += 4 ;

		return true ;
	}
	else if ((m_code == 0x30317762) && (rest >= 4))//ÒôÆµÖ¡//((m_code == 0x01FA) && (rest >= 4))//ÒôÆµÖ¡
	{
		m_FrameInfo = m_FrameInfoList.GetFreeNote() ;
		m_FrameInfo->nType = DH_FRAME_TYPE_AUDIO ;				
		m_FrameInfo->nEncodeType = 15 ;//AVI_AUDIO 15
		m_FrameInfo->nBitsPerSample = m_bitsPerSample;
		m_FrameInfo->nSamplesPerSecond = m_samplesPerSecond;
		m_FrameInfo->nChannels = 1;
		m_FrameInfo->nFrameLength = m_frameLen = m_bufptr[1] <<8 | m_bufptr[0] ;		
		m_FrameInfo->pHeader = m_bufptr - 4 ;
		m_FrameInfo->pContent = m_bufptr + 4 ;
		m_FrameInfo->nLength  = m_frameLen + 8 ;
		rest -= 4 ;
		m_bufptr += 4 ;	

		return true ;
	}

	return false ;
}

inline int log2bin(unsigned int value) 
{
	int n = 0;
	while (value) {
		value>>=1;
		n++;
	}
	return n;
}

#define MMAX(a,b) ((a)>(b)?(a):(b))

inline
bool AVIStream::CheckIfFrameValid()
{
	static const unsigned int AUDIOCODE = 0x30317762 ;
	static const unsigned int VIDEOCODE = 0x30306463 ;
	int i = 4 ;
	m_code = 0 ;
	
	while (rest >0 && i--)
	{
		m_code = m_code << 8 | *m_bufptr++ ;
		rest-- ;
		if (m_code != (AUDIOCODE >> (i*8)) && m_code != (VIDEOCODE >> (i*8)))
		{
			return false ;
		}
	}

	if (m_FrameInfo->nType == DH_FRAME_TYPE_VIDEO)
	{
		int flag = 0;
		unsigned char* pos = m_FrameInfo->pContent;
		for (int i = 0; i < 100 && i < m_FrameInfo->nFrameLength-3; i++)
		{
			if (pos[i] == 0 && pos[i+1] == 0 && pos[i+2] == 1 && (pos[i+3]&0x1f) == 7)
			{
				flag = 1;
				break;
			}
		}

		m_FrameInfo->nSubType = flag ? DH_FRAME_TYPE_VIDEO_I_FRAME:DH_FRAME_TYPE_VIDEO_P_FRAME;
	}

	return true ;
}
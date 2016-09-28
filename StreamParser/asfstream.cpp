#include "asfstream.h"

ASFStream::ASFStream(unsigned char* rawBuf, int packetlen, int videoID):StreamParser(rawBuf)
{
	m_packetlen = packetlen;
	m_videoID = videoID;
	m_newstream = new NewStream(m_newstreamBuf);
	m_tmpLen = 0;
}

ASFStream::~ASFStream()
{
}

inline
bool ASFStream::CheckSign(const unsigned int& Code)
{
	if ((Code&0xffff0000) == 0x115D0000 || (Code&0xffff0000) == 0x105D0000)
	{
		return true ;
	}

	return false ;
}

inline
bool ASFStream::ParseOneFrame()
{
	if (rest < 21)
	{
		return false;
	}

	m_FrameInfo = m_FrameInfoList.GetFreeNote() ;
	m_frameLen = m_packetlen - 25;			
	m_FrameInfo->pHeader = m_bufptr - 4 ;			
	m_FrameInfo->pContent = m_bufptr + 21 ;
	m_FrameInfo->nLength  = m_packetlen ;

	m_FrameInfo->nFrameLength = m_frameLen - *(unsigned short*)(m_FrameInfo->pHeader + 2);

	rest -= 21 ;
	m_bufptr += 21;

	return true ;
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
bool ASFStream::CheckIfFrameValid()
{
	static const unsigned int PACKET_1= 0x115D0000;
	static const unsigned int PACKET_2= 0x105D0000;

	int i = 4 ;
	m_code = 0 ;

	while (rest >0 && i--)
	{
		m_code = m_code << 8 | *m_bufptr++ ;
		rest-- ;
		
		if (i <= 1)
		{
			continue;
		}

		if (m_code != (PACKET_1 >> (i*8)) && m_code != (PACKET_2 >> (i*8)))
		{
			return false ;
		}
	}

	return true ;
}

DH_FRAME_INFO* ASFStream::GetNextFrame()
{
	DH_FRAME_INFO* m_tmp_Frameinfo ;
	static unsigned char AUDIOHeader[8] = {0x00,0x00,0x01,0xF0,14,0x02,0x00,0x00};

	while((m_tmp_Frameinfo=m_FrameInfoList.GetDataNote()) != NULL)
	{
		if (m_tmp_Frameinfo->nFrameLength == 0 || m_tmp_Frameinfo->nLength == 0)
		{
			continue;
		}

		bool bMultiPacket = m_tmp_Frameinfo->pHeader[0] & 0x01;
		if (bMultiPacket == false)
		{
			int streamID = *(unsigned char*)(m_tmp_Frameinfo->pContent-15) & 0x1F;
			if (streamID != m_videoID)//ÒôÆµÖ¡
			{
				int len = 320;
				AUDIOHeader[6] = len & 0xff;
				AUDIOHeader[7] = len >> 8;
				memcpy(m_tmpBuf+m_tmpLen, AUDIOHeader, 8);
				m_tmpLen += 8;
			}
			memcpy(m_tmpBuf+m_tmpLen, m_tmp_Frameinfo->pContent, m_tmp_Frameinfo->nFrameLength);
			m_tmpLen += m_tmp_Frameinfo->nFrameLength;
		}
		else if (bMultiPacket == true)
		{
			int iNum = m_tmp_Frameinfo->pHeader[10] & 0x1F;
			int iPos = 1;
			for (int i = 0; i < iNum; i++)
			{
				unsigned short wLen = *(unsigned short*)(m_tmp_Frameinfo->pContent+iPos);
			
				int streamID = *(unsigned char*)(m_tmp_Frameinfo->pContent+iPos+sizeof(unsigned short)-17) & 0x1F;
				if (streamID != m_videoID)//ÒôÆµÖ¡
				{
					int len = wLen;
					AUDIOHeader[6] = len & 0xff;
					AUDIOHeader[7] = len >> 8;
					memcpy(m_tmpBuf+m_tmpLen, AUDIOHeader, 8);
					m_tmpLen += 8;
				}

				memcpy(m_tmpBuf+m_tmpLen, m_tmp_Frameinfo->pContent+iPos+sizeof(unsigned short), wLen);
				m_tmpLen += wLen;
				iPos += wLen + sizeof(unsigned short) + 15;
			}
		}

		m_FrameInfoList.AddToFreeList(m_tmp_Frameinfo) ;
		
	}

	if (m_tmpLen != 0)
	{
		m_newstream->ParseData(m_tmpBuf, m_tmpLen);
		m_tmpLen = 0;
	}

	return m_newstream->GetNextFrame();
}

DH_FRAME_INFO* ASFStream::GetNextKeyFrame()
{
	return NULL;
}
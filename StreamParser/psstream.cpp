// newstream.cpp: implementation of the newstream class.
//
//////////////////////////////////////////////////////////////////////
#include "psstream.h"
#include "decSPS.h"


PSStream::PSStream(unsigned char* rawBuf):StreamParser(rawBuf)
{
	m_PSBuf = m_buffer+16;
	m_PSRemainLen = 0;
	m_pLastFrame = NULL;
	memset(&m_tmpFrame, 0, sizeof(DH_FRAME_INFO));
	videoType = DH_ENCODE_UNKNOWN;
	audioType = DH_ENCODE_UNKNOWN;
	psmPos = 0;
	setPsm = 0;
	m_tmpFrame.nType = DH_FRAME_TYPE_VIDEO;
	m_tmpFrame.nSubType = DH_FRAME_TYPE_VIDEO_P_FRAME;
	m_beginTime = time(NULL);
}

PSStream::~PSStream()
{
}


int PSStream::Reset(int level)
{
	StreamParser::Reset(level);
	m_PSRemainLen = 0;
	m_pLastFrame = NULL;
	memset(&m_tmpFrame, 0, sizeof(DH_FRAME_INFO));
	m_tmpFrame.nType = DH_FRAME_TYPE_VIDEO;
	m_tmpFrame.nSubType = DH_FRAME_TYPE_VIDEO_P_FRAME;
	videoType = DH_ENCODE_UNKNOWN;
	audioType = DH_ENCODE_UNKNOWN;
	psmPos = 0;
	setPsm = 0;
	return 0;
}

inline
bool PSStream::CheckSign(const unsigned int& Code)
{
	if (Code == 0x01BA || Code == 0x01E0  || Code == 0x01C0 || Code == 0x01BD )
	{
		return true ;
	}

	return false ;
}

inline
bool PSStream::ParseOneFrame()
{
	int irest = rest;
	int psmLen,desLen,eleLen;
	bool ifIFrame = false;
	unsigned char *psmPtr;
	unsigned int code = m_code;
	unsigned char *pBuf = m_bufptr;

	if (m_code == 0x01BA)
	{
		while (irest--)
		{
			if (code == 0x01BC)
			{
				ifIFrame = true;
				setPsm = 1;
			}
			else if(code == 0x01BB)
			{
				ifIFrame = true;
				setPsm = 0; 
			}
			else if (code == 0x01E0 || code == 0x01C0)
			{
			    setPsm = 0; 
				break;
			}
            
			if(setPsm && psmPos<96)
			{
				psmBuf[psmPos] = *pBuf;
				psmPos++;
			}

			code = code << 8 | *pBuf++;
		}
	}
	
	if (irest < 5 && irest < (5+pBuf[4]))
	{
		return false;
	}

	if(psmPos)
	{
		psmLen = psmBuf[0]<<8 | psmBuf[1];
		if((psmLen)>(96-2+4))
			return false;
		desLen = (psmBuf[4] << 8) + psmBuf[5];
		psmPtr = psmBuf + 6 + desLen;
		eleLen = psmPtr[0]<<8 | psmPtr[1];
		psmPtr += 2;
		while(eleLen)
		{
			if(0xe0==*(psmPtr+1))
			{
				if(0x1b==*psmPtr)
					videoType = DH_ENCODE_VIDEO_H264;
				else if(0x24==*psmPtr)
					videoType = DH_ENCODE_VIDEO_HEVC;
			}
			else if(0xc0==*(psmPtr+1))	
			{
				if(0x90==*psmPtr)
					audioType= DH_ENCODE_AUDIO_G711A;
				else if(0x0F==*psmPtr)
					audioType = DH_ENCODE_AUDIO_AAC;
			}
			
			psmPtr+=2;
			desLen = (psmPtr[0] << 8) + psmPtr[1];
			psmPtr+=(desLen+2);
			eleLen -= (desLen+4);
		}
		psmPos = 0;
	}

	if (code == 0x01E0 || code == 0x01C0 || code == 0x01BD)
	{		
		m_FrameInfo = m_FrameInfoList.GetFreeNote() ;
		m_FrameInfo->pHeader = m_bufptr - 4 ;
		m_FrameInfo->nFrameLength = m_frameLen = (pBuf[0]<<8|pBuf[1]) - (3+pBuf[4]);
		if (3+pBuf[4] > (pBuf[0]<<8|pBuf[1]))
		{
			m_FrameInfo->nFrameLength = m_frameLen = (pBuf[0]<<8|pBuf[1]);
		}

		if (code == 0x01E0 && m_code == 0x01BA)
		{
			m_FrameInfo->nTimeStamp = (m_bufptr[1] << 24) | (m_bufptr[2]<< 16) | (m_bufptr[3]<< 8) | m_bufptr[4];
			if (m_FrameInfo->nTimeStamp == 0)
			{
				m_FrameInfo->nTimeStamp=3600;
			}
		}
		else
		{
			m_FrameInfo->nTimeStamp = 0;
		}
		m_FrameInfo->nParam1 = m_bufptr[0];

		m_FrameInfo->nLength = m_frameLen + ((rest == irest)? 0 : rest - irest - 1) + pBuf[4] + 9;
		m_FrameInfo->pContent = m_FrameInfo->pHeader + m_FrameInfo->nLength - m_frameLen;
		m_bufptr = m_FrameInfo->pContent;
		
		rest -= (m_FrameInfo->pContent - m_FrameInfo->pHeader - 4);

		if (code == 0x01E0)
		{
			m_FrameInfo->nType = DH_FRAME_TYPE_VIDEO ;
			m_FrameInfo->nSubType = ifIFrame ? DH_FRAME_TYPE_VIDEO_I_FRAME : DH_FRAME_TYPE_VIDEO_P_FRAME;
			m_FrameInfo->nEncodeType = videoType;
		}
		else if (code == 0x01BD)
		{
			m_FrameInfo->nType = DH_FRAME_TYPE_VIDEO ;
			m_FrameInfo->nSubType = DH_FRAME_TYPE_VIDEO_P_FRAME;
			m_FrameInfo->nEncodeType = videoType;
		}
		else
		{
			m_FrameInfo->nType = DH_FRAME_TYPE_AUDIO ;	
			m_FrameInfo->nEncodeType = audioType;
			m_FrameInfo->nBitsPerSample = 16;
			m_FrameInfo->nSamplesPerSecond = 8000;	
			m_FrameInfo->nChannels = 1;
		}
	}

	return true ;
}

inline
bool PSStream::CheckIfFrameValid()
{
	static const unsigned int PSCODE = 0x000001BA ;
	static const unsigned int PS64K_Code = 0x01E0;
	static const unsigned int PSAudio_Code = 0x01C0;
	static const unsigned int PSBD_Code = 0x01BD;
	int i = 4 ;
	m_code = 0 ;

	while (rest >0 && i--)
	{
		m_code = m_code << 8 | *m_bufptr++ ;
		rest-- ;
		if (m_code != (PSCODE >> (i*8)) 
			&& m_code != (PS64K_Code >> (i*8)) 
			&& m_code != (PSBD_Code >> (i*8)) 
			&& m_code != (PSAudio_Code >> (i*8)))
		{
			return false ;
		}
	}

	return true ;
}

DH_FRAME_INFO* PSStream::GetNextFrame()
{
	DH_FRAME_INFO* frame;

	if (m_pLastFrame)
	{
		frame = m_pLastFrame;
		m_pLastFrame = NULL;
	}
	else
	{
		frame = m_FrameInfoList.GetDataNote();
		if (frame == NULL)
		{
			return NULL;
		}
	}
	
	if ((frame->nTimeStamp != 0 || frame->nType == DH_FRAME_TYPE_AUDIO) && m_PSRemainLen)//前一帧已完整，时间戳变了
	{
		m_pLastFrame = frame;
		frame = m_FrameInfoList.GetFreeNote() ;
		memcpy(frame, &m_tmpFrame, sizeof(DH_FRAME_INFO));
		m_tmpFrame.nSubType = DH_FRAME_TYPE_VIDEO_P_FRAME;
		frame->nFrameLength = m_PSRemainLen;
		m_PSRemainLen = 0;
		makeVideoFrame(frame);
	}
	else
	{
		if (frame->nType == DH_FRAME_TYPE_VIDEO)
		{
			unsigned char *pBuf = frame->pContent ;
			int bCompleteFrame = 0;

			if (m_PSRemainLen + frame->nFrameLength < PSIFRAMELEN-16)
			{
				memcpy(m_PSBuf+m_PSRemainLen, frame->pContent, frame->nFrameLength);
				m_PSRemainLen += frame->nFrameLength;
			}

			if (frame->nSubType == DH_FRAME_TYPE_VIDEO_I_FRAME)
			{
				memcpy(&m_tmpFrame, frame, sizeof(DH_FRAME_INFO));
			}

			frame->nLength = 0;
			frame->nType = DH_FRAME_TYPE_DATA;
		
		}//end of if (m_FrameInfo->nType == DH_FRAME_TYPE_VIDEO)
		else if (frame->nType == DH_FRAME_TYPE_AUDIO)
		{
			makeAudioFrame(frame);
		}
	}

	m_FrameInfoList.AddToFreeList(frame) ;
	return frame;
}

int PSStream::makeVideoFrame(DH_FRAME_INFO* frame)
{
	static const unsigned int IFrameTag = 0xFD010000;
	static const unsigned int PFrameTag = 0xFC010000;
	static const int IFrameHeaderLen = 16;
	static const int PFrameHeaderLen = 8;

	frame->nFrameRate = 25;

	if (frame->nSubType == DH_FRAME_TYPE_VIDEO_I_FRAME)
	{
		memset(m_buffer, 0, IFrameHeaderLen);
		memcpy(m_buffer, &IFrameTag, 4);
	
		
		frame->pHeader = m_buffer;
		frame->pContent = m_PSBuf;
		frame->nLength = frame->nFrameLength + IFrameHeaderLen;
		int iret = decsps(frame->pContent, frame->nFrameLength > 100 ? 100 : frame->nFrameLength, 
			           (unsigned int*)&frame->nWidth, (unsigned int*)&frame->nHeight, &frame->nFrameRate);
		
        m_buffer[4] = 0;
		m_buffer[5] = frame->nFrameRate;	
		m_buffer[6] = frame->nWidth / 8;
		m_buffer[7] = frame->nHeight / 8;

		unsigned char* pSrc = frame->pContent;
		if (pSrc[0] == 0 && pSrc[1] == 0 && pSrc[2] == 0 && pSrc[3] == 1 && pSrc[4] == 6 && 
			pSrc[5] == 'A' && pSrc[6] == 'I' && pSrc[7] == 'P' && pSrc[8] == 'U')
		{
			unsigned long dt = pSrc[9] | pSrc[10] <<8 | pSrc[11] << 16 | pSrc[12] << 24;
			frame->nSecond = dt & 0x3f;
			frame->nMinute = (dt >> 6) & 0x3f;
			frame->nHour = (dt >> 12) & 0x1f;
			frame->nDay = (dt >> 17) & 0x1f;
			frame->nMonth = (dt >> 22) & 0xf;
			frame->nYear = 2000 + (dt >> 26);

			tm tmp_time ;
			tmp_time.tm_hour = frame->nHour ;
			tmp_time.tm_min  = frame->nMinute ;
			tmp_time.tm_sec  = frame->nSecond ;
			tmp_time.tm_mday = frame->nDay ;
			tmp_time.tm_mon  = frame->nMonth - 1 ;
			tmp_time.tm_year = frame->nYear - 1900 ;
			frame->nTimeStamp = mktime(&tmp_time) ;
			m_tmpFrame.nFrameRate = pSrc[13];
		}
		memcpy(m_buffer+8,  &frame->nTimeStamp, 4);
		memcpy(m_buffer+12, &frame->nFrameLength, 4);
	}
	else
	{
		memcpy(m_buffer+8, &PFrameTag, 4);
		memcpy(m_buffer+12, &frame->nFrameLength, 4);
		frame->pHeader = m_buffer+8;
		frame->pContent = m_PSBuf;
		frame->nLength = frame->nFrameLength + PFrameHeaderLen;
		if (frame->nFrameLength == 8 && 
			frame->pContent[0] == 0 && 
			frame->pContent[1] == 0	&& 
			frame->pContent[2] == 0 && 
			frame->pContent[3] == 1 && 
			frame->pContent[4] == 9)
		{
			frame->nType = DH_FRAME_TYPE_DATA;
			frame->nLength = 0;
		}
	}

	return 0;
}

unsigned int PSStream::adts_sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0};

int PSStream::makeAudioFrame(DH_FRAME_INFO* frame)
{
	static const unsigned int AudioTag  = 0xF0010000;
	static const int AudioHeaderLen  = 8;
	unsigned char *sample;
	
	memcpy(m_audioBuf, &AudioTag, 4);
	m_audioBuf[4] = 14;
	m_audioBuf[5] = 2;
	memcpy(m_audioBuf+6, &frame->nFrameLength, 2);
	memcpy(m_audioBuf+8, frame->pContent, frame->nFrameLength);

	frame->pHeader = m_audioBuf;
	sample = frame->pContent = m_audioBuf+8;
	frame->nLength = frame->nFrameLength + AudioHeaderLen;

	if(DH_ENCODE_AUDIO_AAC==frame->nEncodeType)
	{
		frame->nSamplesPerSecond = adts_sample_rates[(sample[2]&0x3c)>>2];;	
		frame->nChannels = 2;
		frame->nBitsPerSample = 16;
	}
	
	return 0;
}

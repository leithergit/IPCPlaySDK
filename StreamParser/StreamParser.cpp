// StreamParser.cpp: implementation of the StreamParser class.
//
//////////////////////////////////////////////////////////////////////
#include "StreamParser.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

StreamParser::StreamParser(unsigned char* rawBuf)
{
	m_rawBuf          = rawBuf ;
	m_bufptr			= m_rawBuf ;
	m_FrameInfo		= NULL ;
	m_code				 = 0 ;
	m_frameLen		 = 0 ;
	m_RemainLen    = 0 ;
	rest					   = 0 ;
	m_preShSeq = 0;
	//2009-1-16刘阳修改start
	m_frameNotCon = false;
	//2009-1-16刘阳修改end
}

StreamParser::~StreamParser()
{

}

DH_FRAME_INFO *StreamParser::GetNextFrame()
{
	m_tmp_Frameinfo = m_FrameInfoList.GetDataNote() ;

	if (m_tmp_Frameinfo != NULL)
	{
		m_FrameInfoList.AddToFreeList(m_tmp_Frameinfo) ;
	}

	return m_tmp_Frameinfo ;
}

DH_FRAME_INFO *StreamParser::GetNextKeyFrame()
{
	while (m_tmp_Frameinfo = m_FrameInfoList.GetDataNote())
	{
		m_FrameInfoList.AddToFreeList(m_tmp_Frameinfo) ;
		if (m_tmp_Frameinfo->nSubType == DH_FRAME_TYPE_VIDEO_I_FRAME)
		{
			return m_tmp_Frameinfo ;
		}
	}

	return NULL ;
}

int StreamParser::Reset(int level)
{	
	if (level == 2)
	{
	}
	else //level == 1
	{
	 	m_frameLen = m_RemainLen = rest = 0 ;
		m_bufptr = m_rawBuf ;
		m_FrameInfoList.Reset() ;
		m_preShSeq = 0;
		//2010-1-18刘阳修改start
		m_frameNotCon = false;
		//2010-1-18刘阳修改end
		if (m_FrameInfo)
		{
			m_FrameInfoList.AddToFreeList(m_FrameInfo);
			m_FrameInfo = NULL ;
		}
	}

	return 0 ;
}

int StreamParser::ParseData(unsigned char *data, int len)
{
	if (data == NULL || len <= 0)
	{
		return -1 ;
	}

	if (m_RemainLen + len > MAX_BUFFER_SIZE)
	{
		m_frameLen = 0 ;
		m_RemainLen = 0 ;
		m_bufptr = m_rawBuf ;
		return 0;
	}

	if (m_rawBuf != m_bufptr)
	{
		memcpy(m_rawBuf, m_bufptr, m_RemainLen) ;
		m_bufptr = m_rawBuf ;
	}

	m_FrameInfoList.Reset() ;//清空有效帧信息链表

	m_code = 0xffffffff;

	if (m_frameLen == 0)//缓冲区中的数据没有完整的帧头信息
	{
		if (m_RemainLen > 0)//如果缓冲区中有上次遗留的数据，把传送进来的数据都拷贝到该缓冲区中
		{
			memcpy(m_rawBuf+m_RemainLen, data, len) ;
			m_RemainLen += len ;
			rest = m_RemainLen ;
		}
		else//直接分析传送进来的数据
		{
			m_bufptr = data ;
			rest = len ;
			m_RemainLen = 0 ;
		}
	}//end of if (m_frameLen == 0)
	else//缓冲区中的数据有完整的帧头信息
	{
		if (len >= m_frameLen)//如果传送进来的数据可以和上次遗留的数据合成完整的一帧数据
		{
			if (m_FrameInfo == NULL)//一般不会发生这种现象
			{
				m_frameLen = 0 ;
				m_RemainLen = 0 ;
				return 0;
			}
			
			memcpy(m_rawBuf+m_RemainLen, data, m_frameLen) ;
			m_bufptr = data+m_frameLen ;
			rest = len - m_frameLen ;
			m_RemainLen += m_frameLen ;
			
			//判断该帧数据是否丢失
			if (CheckIfFrameValid() == false)
			{
				m_FrameInfo->nFrameLength = 0 ;	
				m_RemainLen = 0 ;
				m_bufptr = data ;
				rest = len ;
			}

			m_frameLen = 0 ;
			m_FrameInfoList.AddToDataList(m_FrameInfo) ;  // iframe into ti
			m_FrameInfo = NULL ;	
		}
		else//继续等待下次传进来的数据
		{
			memcpy(m_rawBuf+m_RemainLen, data, len) ;
			m_RemainLen += len ;
			m_frameLen -= len ;
			return 0 ;
		}		
	}

	while (1)
	{
		if (rest > 0 && rest < 16)//如果所剩数据量过小，保存数据，等待下一次分析
		{		
			int tmp_flag = 0 ;
			if (m_code != 0xffffffff)
			{
	 			m_code = (m_code>>24) | (((m_code>>16)&0xff)<<8) | (((m_code>>8)&0xff)<<16) | ((m_code&0xff)<<24) ;
				memcpy(m_rawBuf+ m_RemainLen,&m_code,sizeof(m_code)) ;
				tmp_flag = 4 ;
			}
			memcpy(m_rawBuf+ m_RemainLen+tmp_flag, m_bufptr, rest) ;
			m_bufptr = m_rawBuf+ m_RemainLen ;
			m_RemainLen = tmp_flag + rest ;
			m_frameLen = 0 ;
			return 0 ;
		}


		//寻找帧头
		while (rest > 0 && (CheckSign(m_code) == false) )
		{
			m_code = m_code << 8 | *m_bufptr++;
			rest--;
		}		

		if (rest == 0)//没有找到帧头
		{
			memcpy(m_rawBuf + m_RemainLen, m_bufptr-4, 4) ;
			m_bufptr = m_rawBuf + m_RemainLen ;
			m_RemainLen = 4 ;
			m_frameLen = 0 ;
			return 0 ;
		}


		//分析数据
		if (ParseOneFrame() == false)
		{
			//剩余数据没有完整的帧头信息，保存，留到下一次分析
	 		m_code = (m_code>>24) | (((m_code>>16)&0xff)<<8) | (((m_code>>8)&0xff)<<16) | ((m_code&0xff)<<24) ;
			if (m_RemainLen + 4 + rest >= MAX_BUFFER_SIZE)
			{
				m_RemainLen = 0;
			}
			memcpy(m_rawBuf+ m_RemainLen,&m_code,sizeof(m_code)) ;
			memcpy(m_rawBuf+ m_RemainLen+sizeof(m_code), m_bufptr, rest) ;
			m_bufptr = m_rawBuf+ m_RemainLen ;
			m_RemainLen = sizeof(m_code) + rest ;
			m_frameLen = 0 ;
			return 0 ;
		}

		//剩余数据中有一帧数据的长度
		if (m_frameLen <= rest)
		{		
			m_bufptr += m_frameLen ;
			rest -= m_frameLen ;

			//判断是否丢数据
			if (CheckIfFrameValid() == false)
			{
				m_FrameInfo->nFrameLength = 0 ;
				m_bufptr -= m_frameLen ;//退回
				rest += m_frameLen ;
			}//end of else//丢数据


			m_FrameInfoList.AddToDataList(m_FrameInfo); 	
			m_FrameInfo = NULL ;
		}
		else if (m_frameLen < MAX_BUFFER_SIZE/2)
		{
			//拷贝包含帧头的剩余数据
			memcpy(m_rawBuf + m_RemainLen, m_bufptr - 
				(m_FrameInfo->nLength-m_frameLen), rest + (m_FrameInfo->nLength-m_frameLen)) ;
			int extLen = m_FrameInfo->pContent - m_FrameInfo->pHeader ;
			m_FrameInfo->pHeader = m_rawBuf ;
			m_FrameInfo->pContent = m_FrameInfo->pHeader + extLen ;
			m_bufptr = m_rawBuf + m_RemainLen ;
			m_RemainLen = rest + (m_FrameInfo->nLength-m_frameLen);
			m_frameLen -= rest ;
			return 0 ;
		}	
		else//如果数据长度大于500K
		{		
			m_FrameInfo->nFrameLength = 0 ;
			m_FrameInfoList.AddToDataList(m_FrameInfo); 		
			m_FrameInfo = NULL ;

	 		m_bufptr = m_rawBuf  ;
			m_frameLen = m_RemainLen = 0 ; 
			return 0 ;
		}
	}//end of while (1)

	m_frameLen = m_RemainLen = 0 ; 
	m_bufptr = m_rawBuf ;

	return 0 ;
}

void StreamParser::AudioInfoOpr(DH_FRAME_INFO* frameinfo, const unsigned char& samplespersecond)
{
 	if (samplespersecond == SAMPLE_FREQ_4000) {
		frameinfo->nSamplesPerSecond = 4000;			
	} else if (samplespersecond == SAMPLE_FREQ_8000) {
		frameinfo->nSamplesPerSecond = 8000;
	} else if (samplespersecond == SAMPLE_FREQ_11025) {
		frameinfo->nSamplesPerSecond = 11025;
	} else if (samplespersecond == SAMPLE_FREQ_16000) {
		frameinfo->nSamplesPerSecond = 16000;
	} else if (samplespersecond == SAMPLE_FREQ_20000) {
		frameinfo->nSamplesPerSecond = 20000;
	} else if (samplespersecond == SAMPLE_FREQ_22050) {
		frameinfo->nSamplesPerSecond = 22050;
	} else if (samplespersecond == SAMPLE_FREQ_32000) {
		frameinfo->nSamplesPerSecond = 32000;
	} else if (samplespersecond == SAMPLE_FREQ_44100) {
		frameinfo->nSamplesPerSecond = 44100;
	} else if (samplespersecond == SAMPLE_FREQ_48000) {
		frameinfo->nSamplesPerSecond = 48000;
	} else {
		frameinfo->nSamplesPerSecond = 8000;
	}	

	if (frameinfo->nEncodeType == DH_ENCODE_AUDIO_PCM8
		|| frameinfo->nEncodeType == DH_ENCODE_AUDIO_TALK)
	{
		frameinfo->nBitsPerSample = 8;
	}
	else
	{
		frameinfo->nBitsPerSample = 16;
	}
}

//chenf20090511-s
int StreamParser::GetFrameDataListSize()
{
	return m_FrameInfoList.GetDataListSize();
}
//chenf20090511-e

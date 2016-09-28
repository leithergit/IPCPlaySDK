#include "rwstream.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "StdMp4Parse.h"

RwStream::RwStream(unsigned char* rawBuf,MPEGTYPE mp4type, DH_FRAME_INFO* AviInfo):StreamParser(rawBuf)
{
	m_datapos_1  = 0;
	m_datapos_2  = 0;
	m_mp4type = mp4type;
	m_lastRemain = 0;
	m_IfFindIFrame = false;

	if (AviInfo)
	{
		memcpy(&m_AviInfo, AviInfo, sizeof(m_AviInfo));
	}
	else
	{
		memset(&m_AviInfo, 0, sizeof(m_AviInfo));
	}

	tmp_IFrameHeadLenth = 0;
}

RwStream::~RwStream()
{
}

bool RwStream::CheckSign(const unsigned int& Code)
{
	return true ;
}

void RwStream::AddFrameInfo(DH_FRAME_INFO* frame_info)
{
 	m_FrameInfoList.AddToDataList(frame_info); 
}

void RwStream::GetShFrameInfo(const DH_FRAME_INFO* shframeinfo)
{
	memcpy(&m_ShFrameInfo, shframeinfo, sizeof(DH_FRAME_INFO)) ;
}

void RwStream::ReSet(int level)
{
	m_FrameInfoList.Reset() ;//清空有效帧信息链表
	
	if (level == 0)
	{
		m_FrameInfo = NULL ;
		m_lastRemain = m_RemainLen = 0 ;
		m_IfFindIFrame = false ;
	}
	else
	{
		if (m_FrameInfo && m_FrameInfo->pHeader != m_rawBuf)
		{
			memcpy(m_rawBuf, m_FrameInfo->pHeader, m_lastRemain) ;
			m_FrameInfo->pHeader = m_FrameInfo->pContent = m_rawBuf ;
		}
		m_RemainLen = m_lastRemain ;
	}
}

int RwStream::ParseData(unsigned char *data, int datalen) 
{
	if (data == 0 || datalen < 0) 
	{ 
		return 0;
	}

	if (m_mp4type != MPEG_SH2)
	{
		if (m_FrameInfo && m_FrameInfo->pHeader != m_rawBuf)
		{
			memcpy(m_rawBuf, m_FrameInfo->pHeader, m_RemainLen) ;
			m_FrameInfo->pHeader = m_FrameInfo->pContent = m_rawBuf ;
		}

		m_FrameInfoList.Reset() ;//清空有效帧信息链表
	}

	m_bufptr = data ;
	m_datapos_1 = 0 ;
	m_datapos_2 = 0 ;
	rest                  = datalen ;

	while (1)
	{
		while (rest > 0 && m_code != 0x01B6 && m_code != 0x01F0 
			&& m_code != 0x01F1 && m_code != 0x30317762)
		{
			m_code = m_code << 8 | *m_bufptr++;
			rest--;
		}	

		if (rest == 0 && m_code != 0x01B6 && m_code != 0x01F0
			&& m_code != 0x01F1
			&& m_code != 0x30317762
			&& m_code != 0x01B6)
		{
			if (m_datapos_2 != 0)
			{
				m_lastRemain = data + datalen - m_FrameInfo->pHeader ;
				if (m_RemainLen + m_lastRemain < MAX_BUFFER_SIZE)
				{
					memcpy(m_rawBuf+m_RemainLen, m_FrameInfo->pHeader, m_lastRemain) ;				
					m_FrameInfo->pHeader = m_FrameInfo->pContent = m_rawBuf + m_RemainLen ;

					if (m_mp4type != MPEG_SH2)
					{
						m_RemainLen = m_lastRemain ;
					}
					else
					{
						m_RemainLen += m_lastRemain ;
					}					
				}
				else
				{
					m_FrameInfo = NULL ;
					m_RemainLen = 0 ;
				}
			}
			else if (m_datapos_1 != 0)
			{				
				if (m_RemainLen + datalen < MAX_BUFFER_SIZE)
				{
					memcpy(m_rawBuf+m_RemainLen, data, datalen) ;
					CheckIfIFrame(m_rawBuf, m_RemainLen+m_datapos_1-4) ;
					m_lastRemain = m_rawBuf+m_RemainLen+datalen - m_FrameInfo->pHeader ;
			
					if (m_mp4type != MPEG_SH2)
					{
						m_RemainLen = m_lastRemain ;
					}
					else
					{
						m_RemainLen += datalen;
					}
				}
				else
				{
					m_FrameInfo = NULL ;
					m_RemainLen = 0 ;
				}
			}
			else
			{
				if (m_RemainLen + datalen < MAX_BUFFER_SIZE)
				{
					memcpy(m_rawBuf+m_RemainLen, data, datalen) ;
					m_RemainLen += datalen ;
					m_lastRemain += datalen ;
				}
				else
				{
					m_FrameInfo = NULL ;
					m_RemainLen = 0 ;
				}
			}

			return 0 ;
		}

		m_code = 0xffffffff ;
		
		if (m_datapos_1 == 0)
		{
			m_datapos_1 = m_bufptr - data ;
			continue ;
		}
		
		if (m_datapos_2 == 0)
		{
			m_datapos_2 = m_bufptr - data - 4 ;
			memcpy(m_rawBuf+m_RemainLen, data, m_datapos_2+4) ;
			
			CheckIfIFrame(m_rawBuf, m_datapos_1+m_RemainLen-4) ;
		  	CheckIfIFrame(m_rawBuf, m_datapos_2+m_RemainLen) ;

			if (m_FrameInfo)
			{
				m_FrameInfo->pHeader =  m_FrameInfo->pContent = data + m_datapos_2 ;
				if (m_FrameInfo->nSubType == DH_FRAME_TYPE_VIDEO_I_FRAME
					&& m_FrameInfo->nType == DH_FRAME_TYPE_VIDEO)
				{
					m_FrameInfo->pHeader =  m_FrameInfo->pContent = data + m_datapos_2 - tmp_IFrameHeadLenth;
				}
			}
			
			m_RemainLen += m_datapos_2 ;
			m_lastRemain += m_datapos_2 ;
			continue ;
		}

		CheckIfIFrame(data, m_bufptr-data-4) ;
	}
	
	return 0 ;
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

int RwStream::CheckIfIFrame(unsigned char* buf, int signpos)
{
	unsigned int tmp_code = 0xFFFFFFFF;	
	int tmp_len = 0;
	unsigned char* tmp_buf = buf + signpos ;
	DH_FRAME_INFO* tmp_frameinfo = NULL ;
	int tmp_IFrameHeadLenth = 0 ;
	unsigned int tmp_id = (*(buf+signpos)) << 24 | (*(buf+signpos+1)) <<16 |
			(*(buf+signpos+2)) << 8 | *(buf+signpos+3) ;

	//寻找I帧，并获取相应信息
	unsigned char* tmp_pos = tmp_buf;
	bool bfindIFrame = false;
	bool bfind_0120 = false;

	while(tmp_pos >= buf && tmp_len < 60 )
	{
		if (tmp_id != 0x01B6)
		{
			break;
		}

		tmp_code = 	(*tmp_pos-- << 24) | (tmp_code >> 8);
		tmp_len ++ ;

		if (tmp_code == 0x0120)
		{
			bfind_0120 = true;
		}
		if ( (tmp_code == 0x00000100 || tmp_code == 0x00000101 || tmp_code == 0x01B0) && bfind_0120)
		{
			tmp_buf = tmp_pos;
			bfindIFrame = true;
		}
	}

	if (bfindIFrame)
	{
		m_IfFindIFrame = true ;

		tmp_IFrameHeadLenth = buf + signpos - tmp_buf - 1;

		if (m_FrameInfo)
		{
			m_FrameInfo->nLength = m_FrameInfo->nFrameLength = tmp_buf - m_FrameInfo->pHeader + 1 ;
		}

		tmp_frameinfo = m_FrameInfoList.GetFreeNote() ;

		tmp_buf += 4 ;
		
		if (m_mp4type == MPEG_SH2)
		{
			tmp_frameinfo->nWidth = m_ShFrameInfo.nWidth ;
			tmp_frameinfo->nHeight = m_ShFrameInfo.nHeight ;
			tmp_frameinfo->nFrameRate = m_ShFrameInfo.nFrameRate ;
			tmp_frameinfo->nHour  = m_ShFrameInfo.nHour ;
			tmp_frameinfo->nMinute = m_ShFrameInfo.nMinute ;
			tmp_frameinfo->nSecond = m_ShFrameInfo.nSecond ;
			tmp_frameinfo->nYear = m_ShFrameInfo.nYear ;
			tmp_frameinfo->nMonth = m_ShFrameInfo.nMonth ;
			tmp_frameinfo->nDay = m_ShFrameInfo.nDay ;
			tmp_frameinfo->nTimeStamp = m_ShFrameInfo.nTimeStamp;
			tmp_frameinfo->nParam1 = 2;
			goto L ;//
		}
		else if (m_mp4type == MPEG_AVI)
		{
			tmp_frameinfo->nWidth = m_AviInfo.nWidth ;
			tmp_frameinfo->nHeight = m_AviInfo.nHeight ;
			tmp_frameinfo->nFrameRate = m_AviInfo.nFrameRate ;
			goto L;
		}

		int hbe = 0;
		while (tmp_code != 0x000001B6)
		{
			tmp_code = tmp_code << 8 | *tmp_buf++ ;
			if (tmp_code == 0x0120)
			{
				if (tmp_buf[1] == 4)
				{
				}
				else if (tmp_buf[1] == 6)
				{
				}
				else if (tmp_buf[1]==0xC8&&tmp_buf[2]==0x88)
				{
					//HB标准码流
					int resolution = (tmp_buf[3]&0x7f)<<9|tmp_buf[4]<<1|tmp_buf[5]>>7;
					int increment = (tmp_buf[5]&0x1f)<<10|tmp_buf[6]<<2|tmp_buf[7]>>6;
						
					tmp_frameinfo->nFrameRate = (float)resolution/increment;
						
				} 
				else if (tmp_buf[0]==0x00&&tmp_buf[1]==0xC8&&tmp_buf[2]==0x08&&tmp_buf[3]==0x80) 
				{
					//NVS标准码流
					tmp_frameinfo->nFrameRate = (tmp_buf[4]&0x0f)<<1|tmp_buf[5]>>7;
				} 
				else if (tmp_buf[0]==0x00 && tmp_buf[1]==0x86)
				{
					// 1404 MPEG4
					unsigned int s1 = tmp_buf[2]<<16|tmp_buf[3]<<8|tmp_buf[4];
					unsigned int rate = (s1>>2)&0xffff;
					if (rate>30) rate = 25;
					
					tmp_frameinfo->nFrameRate = rate;
				}
				else if (tmp_buf[0]==0x00&&tmp_buf[1]==0xCA)
				{
					hbe = 1;
					// HBE MPEG4
					tmp_frameinfo->nFrameRate = (tmp_buf[5]>>3)&0x1F;
				}
				else
				{
					tmp_frameinfo->nWidth = 352;
					tmp_frameinfo->nHeight = 288;
					continue;
				}
				
				int nRet = ParseStdMp4(tmp_buf , 100 , &tmp_frameinfo->nWidth , &tmp_frameinfo->nHeight);				
				if( nRet < 0 )
				{
					tmp_frameinfo->nWidth = 352;
					tmp_frameinfo->nHeight = 288;					
				}

			}//end of if (tmp_code == 0x0120)
			else if (tmp_code == 0x01B3)
			{
				tmp_frameinfo->nHour	= tmp_buf[0]>>3;
				tmp_frameinfo->nMinute	= (tmp_buf[0]&0x07)<<3 | tmp_buf[1]>>5;
				tmp_frameinfo->nSecond	= (tmp_buf[1]&0x0f)<<2 | tmp_buf[2]>>6;
				tmp_frameinfo->nTimeStamp = (tmp_frameinfo->nHour*3600+
					tmp_frameinfo->nMinute*60+tmp_frameinfo->nSecond)/**1000*/;

			}//end of else if (tmp_code == 0x01B3)
			else if (tmp_code == 0x01B2)
			{				
				if (hbe)
				{
					continue;
				}

				tmp_frameinfo->nFrameRate = tmp_buf[6];
				if (tmp_frameinfo->nFrameRate> 100)
				{
					if (tmp_frameinfo->nFrameRate == 0xFF)
					{
						tmp_frameinfo->nFrameRate = 1;
					}
					else
					{
						tmp_frameinfo->nFrameRate = 25;
					}
				}	

			}//end of else if (tmp_code == 0x01B2)
		}//end of while (tmp_code != 0x000001B6)
	}//end of if (bfindIFrame)

L:
	if (m_FrameInfo)
	{
		if (tmp_frameinfo == NULL)
		{
			if (m_IfFindIFrame)
			{
				m_FrameInfo->nLength = m_FrameInfo->nFrameLength = 
					buf + signpos - m_FrameInfo->pHeader ;
			}
			else
			{
				m_FrameInfo->nFrameLength = 0 ;
			}
		}
		
		if (m_FrameInfo->nType == DH_FRAME_TYPE_DATA)
		{
			unsigned char* pBuf = m_FrameInfo->pHeader + 4;
			m_FrameInfo->nSubType = pBuf[0];
			int opened = pBuf[1];
			if (m_FrameInfo->nSubType==1 || m_FrameInfo->nSubType==2)//字符叠加
			{
				if (opened)
				{
					int xpos,ypos;
					int num = pBuf[2]|pBuf[3]<<8;
					xpos=pBuf[4]|pBuf[5]<<8;
					ypos=pBuf[6]|pBuf[7]<<8;
					xpos*=16;
					ypos*=16;
					m_FrameInfo->nParam1 = num;
					m_FrameInfo->nWidth = xpos;
					m_FrameInfo->nHeight = ypos;
				}
				m_FrameInfo->nParam2 = opened;
				m_FrameInfo->pContent = m_FrameInfo->pHeader + 12;
				m_FrameInfo->nFrameLength = m_FrameInfo->nLength - 12;
			}
			else if (m_FrameInfo->nSubType == 6)//区域遮挡
			{
				if (opened)
				{
					m_FrameInfo->nYear = pBuf[2] | pBuf[3] << 8;//SX
					m_FrameInfo->nMonth = pBuf[4] | pBuf[5] << 8;//SY
					m_FrameInfo->nWidth = pBuf[6] | pBuf[7] << 8;
					m_FrameInfo->nHeight = pBuf[8] | pBuf[9] << 8;

					m_FrameInfo->nHour = pBuf[10];
					m_FrameInfo->nMinute = pBuf[11];
					m_FrameInfo->nSecond = pBuf[12];
				}

				m_FrameInfo->nParam2 = opened;
				m_FrameInfo->pContent = m_FrameInfo->pHeader;
				m_FrameInfo->nFrameLength = m_FrameInfo->nLength;
			}
			else
			{
				m_FrameInfo->nParam2 = 0;
				m_FrameInfo->nFrameLength = 2;
			}
		}
		else if (m_FrameInfo->nType == DH_FRAME_TYPE_AUDIO)
		{
			if (m_mp4type == MPEG_AVI)
			{
				m_FrameInfo->nEncodeType = 15;//#define AVI_AUDIO   15
				m_FrameInfo->nChannels = 1 ;
				m_FrameInfo->pContent = m_FrameInfo->pHeader + 8 ;
				m_FrameInfo->nBitsPerSample = m_AviInfo.nBitsPerSample;
				m_FrameInfo->nSamplesPerSecond = m_AviInfo.nSamplesPerSecond;
				int len = (m_FrameInfo->pHeader[5] << 8) | m_FrameInfo->pHeader[4] ;

				if (len < 32)
				{
					m_FrameInfo->nLength = 0;
					m_FrameInfo->nType = DH_FRAME_TYPE_DATA;
				}
				else
				{
					m_FrameInfo->nFrameLength = len;
				}
			}
			else
			{
				m_FrameInfo->nEncodeType = m_FrameInfo->pHeader[4] ;
				m_FrameInfo->nChannels = 1 ;
				m_FrameInfo->nFrameLength -= 8 ;
				m_FrameInfo->pContent = m_FrameInfo->pHeader + 8 ;
 		
				int tmpAudioLen = (m_FrameInfo->pHeader[7] << 8) | m_FrameInfo->pHeader[6] ;
				m_FrameInfo->nFrameLength = tmpAudioLen;
				AudioInfoOpr(m_FrameInfo, m_FrameInfo->pHeader[5]) ;
			}
		
		}//end of if (m_FrameInfo->nType == DH_FRAME_TYPE_AUDIO)
		else
		{
			
			m_FrameInfo->nEncodeType = DH_ENCODE_VIDEO_MPEG4 ;
		}

		m_FrameInfoList.AddToDataList(m_FrameInfo) ;
	}

	if (tmp_frameinfo == NULL)
	{
		m_FrameInfo = m_FrameInfoList.GetFreeNote() ;
		m_FrameInfo->nSubType = DH_FRAME_TYPE_VIDEO_P_FRAME ;
		m_FrameInfo->pHeader =  m_FrameInfo->pContent = buf + signpos ;
	}
	else
	{
		m_FrameInfo = tmp_frameinfo ;
		m_FrameInfo->nSubType = DH_FRAME_TYPE_VIDEO_I_FRAME ;
		m_FrameInfo->nParam1 = 0;
		m_FrameInfo->pHeader =  m_FrameInfo->pContent = buf + signpos -  tmp_IFrameHeadLenth ;
	}
	
	m_FrameInfo->nType = (tmp_id == 0x01B6) ? DH_FRAME_TYPE_VIDEO :
		((tmp_id == 0x01F1) ? DH_FRAME_TYPE_DATA: DH_FRAME_TYPE_AUDIO) ;


	return 0 ;
}
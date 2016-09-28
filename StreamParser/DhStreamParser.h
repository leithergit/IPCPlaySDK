// DhStreamParser.h: interface for the DhStreamParser class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DHSTREAMPARSER_H__4CB30E13_2FFF_4236_AEE3_E7BD20C8C173__INCLUDED_)
#define AFX_DHSTREAMPARSER_H__4CB30E13_2FFF_4236_AEE3_E7BD20C8C173__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define SAMPLE_FREQ_4000	1
#define SAMPLE_FREQ_8000	2
#define SAMPLE_FREQ_11025	3
#define SAMPLE_FREQ_16000	4
#define SAMPLE_FREQ_20000	5
#define SAMPLE_FREQ_22050	6
#define SAMPLE_FREQ_32000	7
#define SAMPLE_FREQ_44100	8
#define SAMPLE_FREQ_48000	9

#define INFOCOUNT_MAX_SIZE		200
#define MAX_BUFFER_SIZE 1638400

// Stream type:
#define DH_STREAM_UNKNOWN			0
#define DH_STREAM_AVI				1
#define DH_STREAM_DHPT				3
#define DH_STREAM_NEW				4
#define DH_STREAM_HB				5
#define DH_STREAM_AUDIO				6
#define  DH_STREAM_PS               7
#define  DH_STREAM_DHSTD            8
#define  DH_STREAM_ASF              9
#define  DH_STREAM_SVAC             10
#define DH_STREAM_XM				13
// Frame Type and SubType
#define DH_FRAME_TYPE_UNKNOWN		0
#define DH_FRAME_TYPE_VIDEO			1
#define DH_FRAME_TYPE_AUDIO			2
#define DH_FRAME_TYPE_DATA			3

#define DH_FRAME_TYPE_VIDEO_I_FRAME	0
#define DH_FRAME_TYPE_VIDEO_P_FRAME	1
#define DH_FRAME_TYPE_VIDEO_B_FRAME	2
#define DH_FRAME_TYPE_VIDEO_S_FRAME	3
#define DH_FRAME_TYPE_DATA_TEXT    5
#define DH_FRAME_TYPE_DATA_INTL    6

// Encode type:
#define DH_ENCODE_UNKNOWN				0
#define DH_ENCODE_VIDEO_MPEG4			1
#define DH_ENCODE_VIDEO_H264			2
#define DH_ENCODE_VIDEO_JPEG            3
#define DH_ENCODE_VIDEO_HEVC            4
#define DH_ENCODE_VIDEO_SVAC            5
#define DH_ENCODE_VIDEO_SVAC_NEW        6
#define DH_ENCODE_AUDIO_PCM8			7	// 8BITS,8K
#define DH_ENCODE_AUDIO_G729			8
#define DH_ENCODE_AUDIO_IMA				9
#define DH_ENCODE_AUDIO_PCM_MULAW		10
#define DH_ENCODE_AUDIO_G721			11
#define DH_ENCODE_AUDIO_PCM8_VWIS		12	// 16BITS,8K
#define DH_ENCODE_AUDIO_ADPCM			13	// 16BITS,8K/16K
#define DH_ENCODE_AUDIO_G711A			14	// 16BITS,8K
#define DH_ENCODE_AUDIO_TALK           30
#define DH_ENCODE_AUDIO_AAC            17

#define STREAMPARSER_RESET_REFIND       0
#define STREAMPARSER_RESET_CONTINUE     1

typedef struct 
{
	unsigned char* pHeader;
	unsigned char* pContent;
	unsigned long nLength;
	unsigned long nFrameLength;
	
	unsigned int nType;
	unsigned int nSubType;
	
	unsigned int nEncodeType; // MPEG4/H264, PCM, MSADPCM, etc.
	
	unsigned long nYear;
	unsigned long nMonth;
	unsigned long nDay;
	unsigned long nHour;
	unsigned long nMinute;
	unsigned long nSecond;
	unsigned long nTimeStamp;
	
	unsigned int  nFrameRate;
	int			  nWidth;
	int			  nHeight;
	unsigned long nRequence;
	
	unsigned int nChannels;
	unsigned int nBitsPerSample;
	unsigned int nSamplesPerSecond;
	
	unsigned long nParam1;		// 扩展用
	unsigned long nParam2;		// 扩展用

	//2010-1-16 刘阳 修改 start
	long nDiscardFrame;			//0:解码显示;1:不解码不显示（用于标示帧序号不正确的P帧）;解码时用于标示是否丢弃不解码该帧
	long nSerial;				//帧序号，用于bell平台
	//2010-1-16 刘阳 修改 end
} DH_FRAME_INFO;

class StreamParser ;

class DhStreamParser  
{
public:
	DhStreamParser(int nStreamType=DH_STREAM_UNKNOWN, int nFlag=0);
	virtual ~DhStreamParser();

	// 输入数据.
	int InputData(unsigned char *pData, unsigned long nDataLength);

	// 同步输出帧数据.
	DH_FRAME_INFO *GetNextFrame();

	// 调用这个等同于重找I帧,或者第一次调用的时候,找I帧.
	DH_FRAME_INFO *GetNextKeyFrame();

	int Reset(int nLevel = 0, int streamtype = DH_ENCODE_UNKNOWN);

	//得到码流类型
	int GetStreamType();
	int GetFrameDataListSize();//chenf20090511-
	
private:
	StreamParser* m_streamParser ;//解码器
	int m_nStreamType;//码流类型

	unsigned char m_buffer[MAX_BUFFER_SIZE];//原始数据缓冲

private:
	int AutoScanStream(unsigned char *pData, unsigned long nDataLength);
};

#endif // !defined(AFX_DHSTREAMPARSER_H__4CB30E13_2FFF_4236_AEE3_E7BD20C8C173__INCLUDED_)

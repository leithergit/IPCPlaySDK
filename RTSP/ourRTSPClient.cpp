#include "ourRTSPClient.h"

#include "rtsp.h"
#include "crtspsession.h"

ourRTSPClient::ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
                             int verbosityLevel,
                             char const* applicationName,
                             portNumBits tunnelOverHTTPPortNum)
    : RTSPClient(env,rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum)
    ,session(0)
{
}

ourRTSPClient::~ourRTSPClient(void)
{
}

ourRTSPClient* ourRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL,
                                        int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
{
    return new ourRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}


StreamClientState::StreamClientState()
    : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0)
{
}

StreamClientState::~StreamClientState()
{
    delete iter;
    if (session != NULL)
    {
        // We also need to delete "session", and unschedule "streamTimerTask" (if set)
        UsageEnvironment& env = session->envir(); // alias

        env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
        Medium::close(session);
    }
}


DummySink* DummySink::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
{
    return new DummySink(env, subsession, streamId);
}

#include "time.h"

DummySink::DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
    : MediaSink(env),
      fSubsession(subsession),
      session(0),
      DUMMY_SINK_RECEIVE_BUFFER_SIZE(256*1024)
{
    fStreamId = strDup(streamId);
    fReceiveBuffer = NULL;

    bTruncated = false;

    //Start();
}

DummySink::~DummySink()
{
    delete[] fReceiveBuffer;
    delete[] fStreamId;

    //Join();
}

void DummySink::init()
{
    if(fSubsession.fmtp_width()>720)
    {
        DUMMY_SINK_RECEIVE_BUFFER_SIZE = 256*1024;
    }
    fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
}

void DummySink::afterGettingFrame(void* clientData,
                                  unsigned frameSize,
                                  unsigned numTruncatedBytes,
                                  struct timeval presentationTime,
                                  unsigned durationInMicroseconds)
{
    DummySink* sink = (DummySink*)clientData;
    try
    {
        sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
    }
    catch (...)
    {

    }
}

void DummySink::onSourceClose(void* clientData)
{
    DummySink* sink = (DummySink*)clientData;
    sink->onSourceClose();
}

#include "decSPS.h"
#include "time.h"

#ifdef SVAC

#define I_FRAME 0xED010000
#define P_FRAME 0xEC010000
#define A_FRAME 0xEA010000

struct SvacHeader
{
    int  _frameType;
    char _encodeType;
    char _channel;
    char _frameRate;
    char _serial;
    union
    {
        struct
        {
            short _width;
            short _height;
        } _video;
        struct
        {
            char _coding;
            char _channel;
            char _sample;
            char _bits;
        } _audio;
    } _param;
    //struct
    //{
    //	int _year:6;
    //	int _month:4;
    //	int _day:5;
    //	int _hour:5;
    //	int _minute:6;
    //	int _second:6;
    //}_stamp;
    int _stamp;
    short _msec;
    char _extended[2];
    int _lenth;
};

#endif
static int makeIFrame(char* head, int width, int height, int rate, int frameLen)
{
    time_t now=time(NULL);
    //time_t now=rtpTime;
    tm *newtime = localtime(&now);
    int year = newtime->tm_year + 1900 - 2000;
    int mon = newtime->tm_mon + 1;
    int day = newtime->tm_mday;
    int hour = newtime->tm_hour;
    int min = newtime->tm_min;
    int sec = newtime->tm_sec;

#ifdef SVAC
    SvacHeader *pHeader = (SvacHeader*)head;
    memset(pHeader,0,sizeof(SvacHeader));
    pHeader->_frameType = I_FRAME;
    pHeader->_encodeType = 2;
    pHeader->_frameRate = rate;
    pHeader->_serial = 0;
    pHeader->_param._video._width = width;
    pHeader->_param._video._height = height;
    pHeader->_stamp = ((year)<<26) | (mon<<22) |(day<<17) |(hour<<12) |(min<<6)|(sec);

    pHeader->_lenth = frameLen;
#else
    static const char IFrameHeader[5] = {0, 0, 0x01, 0xFD, 0};
    memcpy(head, IFrameHeader, 5);
    head[5] = rate==0?25:rate;
    head[6] = width/8;
    head[7] = height/8;

    unsigned int timestamp = ( sec & 0x3f)          |
                             ((min & 0x3f) << 6)	|
                             ((hour& 0x1f) << 12)	|
                             ((day & 0x1f) << 17)	|
                             ((mon & 0x0f) << 22)	|
                             ((year& 0x3f) << 26)	;
    memcpy(head + 8,  &timestamp, 4);
    memcpy(head + 12, &frameLen, 4);

    //printf("I frame, len=%d\n", frameLen);
#endif

    return 0;
}

static int makePFrame(char* head, int width, int height, int rate, int frameLen)
{
#ifdef SVAC
    time_t now=time(NULL);
    //time_t now=rtpTime;
    tm *newtime = localtime(&now);
    int year = newtime->tm_year + 1900 - 2000;
    int mon = newtime->tm_mon + 1;
    int day = newtime->tm_mday;
    int hour = newtime->tm_hour;
    int min = newtime->tm_min;
    int sec = newtime->tm_sec;

    SvacHeader *pHeader = (SvacHeader*)head;
    memset(pHeader,0,sizeof(SvacHeader));
    pHeader->_frameType = P_FRAME;
    pHeader->_encodeType = 2;
    pHeader->_frameRate = rate;
    pHeader->_serial = 0;
    pHeader->_param._video._width = width;
    pHeader->_param._video._height =height;
    pHeader->_stamp = ((year)<<26) | (mon<<22) |(day<<17) |(hour<<12) |(min<<6)|(sec);
    pHeader->_lenth = frameLen;
#else
    static const char PFrameHeader[4] = {0, 0, 0x01, 0xFC};
    memcpy(head, PFrameHeader, 4);
    memcpy(head + 4, &frameLen, 4);
#endif

    return 0;
}

//static int makeAudioFrame(char* head, int frameLen)
//{
//	static const char AudioFrameHeader[4] = {0, 0, 0x01, 0xF0};
//	memcpy(head, AudioFrameHeader, 4);
//	head[4]=14;
//	head[5]=2;
//	short audioLen=frameLen;
//	memcpy(head + 6, &frameLen, 2);
//
//	return 0;
//}
// If you don't want to see debugging output for each received frame, then comment out the following line:
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1

FILE* pFile = NULL;

void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned /*durationInMicroseconds*/)
{
    if (numTruncatedBytes>0)
    {
        int newSize = DUMMY_SINK_RECEIVE_BUFFER_SIZE * 2;
        u_int8_t* tmp = new u_int8_t[newSize];
        if (tmp)
        {
            memcpy(tmp,fReceiveBuffer,DUMMY_SINK_RECEIVE_BUFFER_SIZE);

            delete[] fReceiveBuffer;
            fReceiveBuffer = tmp;
            DUMMY_SINK_RECEIVE_BUFFER_SIZE = newSize;
        }
    }

    //unsigned int rtpTime = frame.getTimeStamp();


    if (session == NULL)
    {
        return;
    }
    if (strcmp(fSubsession.mediumName(), "video") != 0)
        return;
// 	session->nVideoWidth = fSubsession.fmtp_width();
// 	session->nVideoHeight = fSubsession.fmtp_height();
// 	session->nVideoFPS = fSubsession.videoFPS();
// 	session->szCodec = (char *)fSubsession.codecName();

#define WQFRAME

    /*
    if (naltype == 7 || naltype == 5 || naltype == 1)
    {
    	if (naltype == 7)
    	{
    		int width = 0, height = 0;
    		int ret = decsps((unsigned char*)pHead, nRTPFrameLen, &width, &height);
    #ifdef WQFRAME
    		makeIFrame(fSubsession.fmtp_header(), width?width:fSubsession.fmtp_width(), height?height:fSubsession.fmtp_height(), fSubsession.videoFPS(), nFrameSize+4);
    #ifdef SVAC
    		session->onRtspData(fSubsession.fmtp_header(), 24);
    #else
    		//session->onRtspData(fSubsession.fmtp_header(), 16);
    		session->onRtspData(fSubsession.fmtp_header(), 16+fSubsession.fmtp_spsLen()+fSubsession.fmtp_ppsLen(),MAKELONG(naltype,0));
    #endif
    #endif
    	}
    	else if (naltype == 5)
    	{
    #ifdef WQFRAME
    		makeIFrame(fSubsession.fmtp_header(), fSubsession.fmtp_width(), fSubsession.fmtp_height(), fSubsession.videoFPS(), nFrameSize+fSubsession.fmtp_spsLen()+fSubsession.fmtp_ppsLen()+4);
    #ifdef SVAC
    		session->onRtspData(fSubsession.fmtp_header(), 24+fSubsession.fmtp_spsLen()+fSubsession.fmtp_ppsLen());
    #else
    		//session->onRtspData(fSubsession.fmtp_header(), 16+fSubsession.fmtp_spsLen()+fSubsession.fmtp_ppsLen());
    		session->onRtspData(fSubsession.fmtp_header(), 16+fSubsession.fmtp_spsLen()+fSubsession.fmtp_ppsLen(),MAKELONG(naltype,0));
    #endif
    #endif
    	}
    	else
    	{
    #ifdef WQFRAME
    		makePFrame(fSubsession.fmtp_header(), fSubsession.fmtp_width(), fSubsession.fmtp_height(), fSubsession.videoFPS(),nFrameSize+4);
    #ifdef SVAC
    		session->onRtspData(fSubsession.fmtp_header(), 24);
    #else
    		//session->onRtspData(fSubsession.fmtp_header(), 8);
    		session->onRtspData(fSubsession.fmtp_header(), 8,MAKELONG(naltype,0));
    #endif
    #endif
    	}

    	static char h[] = {0,0,0,1};

    	session->onRtspData((char*)h, 4,MAKELONG(naltype,1));

    }
    else
    {
    	//printf("naltype=%d\n", naltype);
    }
    */
    if (bFirstFrame)
    {
        unsigned char start_code[4] = { 0x00, 0x00, 0x00, 0x01 };
        bFirstFrame = false;
        unsigned int num;
        SPropRecord *sps = parseSPropParameterSets(fSubsession.fmtp_spropparametersets(), num);
        // For H.264 video stream, we use a special sink that insert start_codes:
        struct timeval tv = { 0, 0 };

        //unsigned char pSPS[1024] = { 0 };
        int nLength = 0;
        for (int i = 0; i < num; i++)
        {
            int nSPSType = sps[i].sPropBytes[0] & 0x1F;
            //TraceMsg("%s NalType = %d\tFrameSize = %d.\n", __FUNCTION__, nSPSType, sps[i].sPropLength);
            session->onRtspData((char*)sps[i].sPropBytes, sps[i].sPropLength);
        }

        delete[] sps;
    }
    int nNalType = fReceiveBuffer[0] & 0x1F;
    /*
    Nal_Value		Nual Type
    0				unused
    1				非IDR图像中不采用数据划分的片段
    2				非IDR图像中A类数据划分片段
    3				非IDR图像中B类数据划分片段
    4				非IDR图像中C类数据划分片段
    5				IDR图像的片
    6				补充增强信息单元(SEI)
    7				序列参数集
    8				图像参数集
    9				分界符
    10				序列结束
    11				码流结束
    12				填充
    13..23			保留
    24..31			不保留(RTP打包时会用到)
    */
    //if (frameSize >= 64*1024)
    //    TraceMsgA("%s Media:%s\tNalType = %d\tFrameSize = %d.\n", __FUNCTION__, fSubsession.mediumName(), nNalType, frameSize);
    session->onRtspData((char*)fReceiveBuffer, frameSize);
    // Then continue, to request the next frame of data:
    continuePlaying();
}

void DummySink::SDPNotify(_SDPAttrib *pSDP)
{
    session->OnSDPNotify(pSDP);
}
void DummySink::onSourceClose()
{
    if(session)
    {
        session->onDisconnect(0, __FUNCTION__);
    }
}

/*

void DummySink::Run()
{
	if(session->isRtspSession())
	{
		running_ = false;
		joined_ = true;
		return;
	}

	do
	{
		__int64 nTick = GetTickCount64();


		if (2000 < (nTick - qLastRecvCheckTime))
		{
			//2000ms收到视频，断线重连
			if(session)
			{
				session->onDisconnect(0);
			}
			//以避免频繁回调
			qLastRecvCheckTime = nTick;
		}
	} while (0);

	Sleep(100);
}
*/
Boolean DummySink::continuePlaying()
{
    if (fSource == NULL) return False; // sanity check (should not happen)

    // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
    fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                          afterGettingFrame, this,
                          onSourceClose, this);
    return True;
}
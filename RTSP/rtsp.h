#ifndef RTSP_H
#define RTSP_H
#include <memory.h>

#ifdef RTSP_LIB
# define RTSP_EXPORT __declspec(dllexport)
#else
# define RTSP_EXPORT __declspec(dllimport)
#endif

struct _SDPAttrib
{
	int nWidth;
	int nHeight;
	int nRate;
	int nSPSLen;
	int nPPSLen;
	int nRTPPayloadFormat;
	int nRTPTimestampFrequency;
	char *pProtocol;
	char *pMediaName;
	char *szCodec;
};
struct _RTSPParam
{
	int nLength;
	int nNalType;
	int nDataType;		// _RTP_Header(0),_NALU(1),_FrameData(2)
	int nWidth;
	int nHeight;
	int nRate;
	char *szCodec;
	_RTSPParam()
	{
		memset(this,0, sizeof(_RTSPParam));
	}
};
typedef void(*PFSDPCallBack)(long hRTSPSession, void *pSDP, long nUser);
typedef void (*PFRtspDataCallBack)(long lHandle, char *pBuffer, int nParam, int nUser);
typedef void (*PFRtspDisConnect)(long lHandle, int nErrorCode,int dwUser,char *szProc);


extern "C"
{
	RTSP_EXPORT long rtsp_login(char* pUrl,
		const char* user,
		const char* pass,
		int httpPort,
		PFRtspDisConnect cbDisconnectCallBack,
								void* pUser,
								int ConnnectTimeout=250);
	RTSP_EXPORT long rtsp_play(const char *pUrl, 
								const char* user, 
								const char* pass, 
								int nNetType, 
								int httpPort, 
								void* cbSDPNotify,
								void* cbDataCallBack,
								void* cbDisconnectCallBack,
								void* pUser,
								int nConnectTimeout=250,
								int nMaxFrameInterval = 2000);
	RTSP_EXPORT long rtsp_setCallback(long lPlayHandle, void* cbSDPNotify, void* cbDataCallBack, void* cbDisconnectCallBack, void* pUser);

	RTSP_EXPORT int	 rtsp_stop(long lHandle);
};


#endif // RTSP_H

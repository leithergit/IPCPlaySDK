#pragma once

#include "platform/Thread.h"
#include "rtsp.h"
#include "BasicUsageEnvironment.hh"
#include <string>

class RTSPClient;
class CRTSPSessionPrivate;
class CRTSPSession : public CThread
{
public:
	CRTSPSession();
	virtual ~CRTSPSession();

	int startRTSPClient(char const* rtspURL, int nHttpPort,
						PFSDPCallBack  cbSDPNotify,
						PFRtspDataCallBack cbDataCallBack, 
						PFRtspDisConnect cbDisconnectCallBack,
						void* pUser, bool bWait,int nConnectTimeout=250,int nMaxFrameinterval = 2000);
	int stopRTSPClient();

	void setNetType(int nType);
	Boolean useTCP();

	void setCallback(PFSDPCallBack cbSDPNotify,PFRtspDataCallBack cbDataCallBack, PFRtspDisConnect cbDisconnectCallBack, void* pUser);

	void HeartBeat();

	bool isRtspSession();
	void OnSDPNotify(_SDPAttrib *pSDP);
	//bool onRtspData(char *pBuffer, int len);
	// nDataType = 0,1,2
	// Hiword 0 = Header,1 = NALU,2 = FRAMEDATA
	// Loword 0 = I frame,1= bp frame
	bool onRtspData(char *pBuffer, int len);
	void onDisconnect(int nErrorCode,char *szProc);
	bool	bRecvFirstFrame = false;
// 	int  nVideoWidth = 0;
// 	int nVideoHeight = 0;
// 	int nVideoFPS = 0;
// 	char *szCodec = nullptr;
protected:
	void Run();
protected:
	int openURL(UsageEnvironment& env, char const* rtspURL, int nHttpPort);

	RTSPClient* m_rtspClient;
	std::string m_rtspUrl;
	int			m_httpPort;
	int			nConnectTimeout;
	int			nMaxFrameInterval;
	char eventLoopWatchVariable;

private:
	CRTSPSessionPrivate* pSessionPrivate;

	friend class CRTSPSessionPrivate;
};
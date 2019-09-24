#pragma once

#include "ourRTSPClient.h"
#include "BasicUsageEnvironment.hh"

#include "rtsp.h"

class CRTSPSessionPrivate
{
public:
	static void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
	static void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
	static void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);

	static void continueAfterGetParameter(RTSPClient* rtspClient, int resultCode, char* resultString);

	static void setupNextSubsession(RTSPClient* rtspClient);
	static void subsessionAfterPlaying(void* clientData);
	static void subsessionByeHandler(void* clientData);
	static void streamTimerHandler(void* clientData);

	static void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);

	static int rtspClientCount;

public:
	bool	m_bWait;
	int		m_nResult;
	HANDLE	m_hEvent;
	bool	m_bHeartbeat;
	double  m_fNextBeat;
	int		m_nHeartBeatInterval;
	double  fLastRecvCheckTime = 0.0f;
	int		m_nCheckNumber;
	int		m_nCallCount = 0;

	Boolean m_bUseTCP;

	CRITICAL_SECTION   csCallBack;
	void OnSDPNotify(_SDPAttrib *pSDPArrib);
	PFSDPCallBack		_rtspSDPNotify;
	PFRtspDataCallBack	_rtspStream;
	PFRtspDisConnect	_rtspDisconnect;
	void*				_user;
	//int					_nConnectTimeout;		// 连接摄像机最大超时值，超过这个值，认为连接失败，建议不超过500ms
	int					_nMaxFrameInterval;		// 允许的最大帧间隔，超过这个时间，将认为是掉线，建议不要超过2000ms
};

#define RTSP_CLIENT_VERBOSITY_LEVEL 1 
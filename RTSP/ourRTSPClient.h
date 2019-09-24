#pragma once

#include "liveMedia.hh"

#include "rtsp.h"
class StreamClientState {
public:
	StreamClientState();
	virtual ~StreamClientState();

public:
	MediaSubsessionIterator* iter;
	MediaSession* session;
	MediaSubsession* subsession;
	TaskToken streamTimerTask;
	double duration;
};

class CRTSPSession;
class ourRTSPClient: public RTSPClient {
public:
	static ourRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel = 0,
		char const* applicationName = NULL,
		portNumBits tunnelOverHTTPPortNum = 0);

protected:
	ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
	// called only by createNew();
	virtual ~ourRTSPClient();

public:
	StreamClientState scs;

	CRTSPSession* session;
};

#include "platform/Thread.h"
class DummySink
	: public MediaSink
{
public:
	static DummySink* createNew(UsageEnvironment& env,
		MediaSubsession& subsession, // identifies the kind of data that's being received
		char const* streamId = NULL); // identifies the stream itself (optional)

	void init();
	void SDPNotify(_SDPAttrib *pSDP);
private:
	DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId);
	// called only by "createNew()"
	virtual ~DummySink();

	static void afterGettingFrame(void* clientData, 
		unsigned frameSize,	
		unsigned numTruncatedBytes,	
		struct timeval presentationTime,		
		unsigned durationInMicroseconds);
	void afterGettingFrame(unsigned frameSize, 
		unsigned numTruncatedBytes,
		struct timeval presentationTime, 
		unsigned durationInMicroseconds);

	

	static void onSourceClose(void* clientData);
	void onSourceClose();

protected:
	//virtual void Run();
private:
	// redefined virtual functions:
	virtual Boolean continuePlaying();

private:
	u_int8_t* fReceiveBuffer;
	MediaSubsession& fSubsession;
	bool bFirstFrame = true;
	char* fStreamId;
	bool bTruncated;
// 	time_t	_lastRecvCheckPkgChangeTime;
// 	__int64	qLastRecvCheckTime;
	int DUMMY_SINK_RECEIVE_BUFFER_SIZE;

public:
	CRTSPSession* session;
};
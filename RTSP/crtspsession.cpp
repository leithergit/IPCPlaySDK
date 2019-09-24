#include "crtspsession.h"
#include "crtspsessionprivate.h"
#include "TimeUtility.h"

CRTSPSession::CRTSPSession(void)
    :CThread("CRTSPSession")
    ,pSessionPrivate(new CRTSPSessionPrivate)
{
    InitializeCriticalSection(&pSessionPrivate->csCallBack);
    pSessionPrivate->_rtspStream = 0;
    pSessionPrivate->_rtspDisconnect = 0;
    pSessionPrivate->_user = 0;

    pSessionPrivate->m_bWait = true;
    pSessionPrivate->m_hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    pSessionPrivate->m_bHeartbeat = false;
    pSessionPrivate->m_fNextBeat = 0.0f;
    pSessionPrivate->m_nCheckNumber = 0;
    pSessionPrivate->fLastRecvCheckTime = 0.0f;
    pSessionPrivate->m_bUseTCP = false;
    pSessionPrivate->m_nHeartBeatInterval = 15;

    m_httpPort = 0;
}

CRTSPSession::~CRTSPSession(void)
{
    stopRTSPClient();
    CloseHandle(pSessionPrivate->m_hEvent);

    DeleteCriticalSection(&pSessionPrivate->csCallBack);
    delete pSessionPrivate;
    pSessionPrivate = NULL;
}

int CRTSPSession::startRTSPClient(char const* rtspURL, int nHttpPort,
                                  PFSDPCallBack  cbSDPNotify,
                                  PFRtspDataCallBack cbDataCallBack,
                                  PFRtspDisConnect cbDisconnectCallBack,
                                  void* pUser, bool bWait, int nConnectTimeout,int MaxFrameInterval)
{
    m_rtspUrl = rtspURL;
    m_httpPort = nHttpPort;
    if(m_rtspUrl.empty())
    {
        return 0;
    }

    EnterCriticalSection(&pSessionPrivate->csCallBack);
    pSessionPrivate->_rtspSDPNotify = cbSDPNotify;
    pSessionPrivate->_rtspStream = cbDataCallBack;
    pSessionPrivate->_rtspDisconnect = cbDisconnectCallBack;
    pSessionPrivate->_user = pUser;
    pSessionPrivate->_nMaxFrameInterval = MaxFrameInterval;
    this->nConnectTimeout = nConnectTimeout;
    LeaveCriticalSection(&pSessionPrivate->csCallBack);

    eventLoopWatchVariable = 0;

    pSessionPrivate->m_bWait = bWait;
    pSessionPrivate->m_nResult = 0;
    if (bWait)
    {
        ResetEvent(pSessionPrivate->m_hEvent);
        Start();
        if(WAIT_TIMEOUT == WaitForSingleObject(pSessionPrivate->m_hEvent,INFINITE))
        {
            return -1;
        }
    }
    else
    {
        Start();
    }

    return pSessionPrivate->m_nResult;
}

int CRTSPSession::stopRTSPClient()
{
    pSessionPrivate->m_bHeartbeat = false;
    EnterCriticalSection(&pSessionPrivate->csCallBack);
    pSessionPrivate->_rtspStream = nullptr;
    pSessionPrivate->_rtspDisconnect = nullptr;
    pSessionPrivate->_rtspSDPNotify = nullptr;
    pSessionPrivate->_user = 0;
    LeaveCriticalSection(&pSessionPrivate->csCallBack);
    eventLoopWatchVariable = 1;
    Join();
    return 0;
}

void CRTSPSession::setNetType(int nType)
{
    pSessionPrivate->m_bUseTCP = nType;
}

Boolean CRTSPSession::useTCP()
{
    return pSessionPrivate->m_bUseTCP;
}

void CRTSPSession::setCallback(PFSDPCallBack cbSDPNotify,PFRtspDataCallBack cbDataCallBack, PFRtspDisConnect cbDisconnectCallBack, void* pUser)
{
    EnterCriticalSection(&pSessionPrivate->csCallBack);
    pSessionPrivate->_rtspSDPNotify = cbSDPNotify;
    pSessionPrivate->_rtspStream = cbDataCallBack;
    pSessionPrivate->_rtspDisconnect = cbDisconnectCallBack;
    pSessionPrivate->_user = pUser;
    LeaveCriticalSection(&pSessionPrivate->csCallBack);
}

#define MAKEUINT64(low,high)	((UINT64)(((UINT)(low)) | ((UINT64)((UINT)(high))) << 32))

void CRTSPSession::HeartBeat()
{
    if (pSessionPrivate->m_bHeartbeat)
    {
        double dfNow = GetExactTime();
        if (dfNow >= pSessionPrivate->m_fNextBeat)
        {
            pSessionPrivate->m_fNextBeat = dfNow + pSessionPrivate->m_nHeartBeatInterval;

            StreamClientState& scs = ((ourRTSPClient*)m_rtspClient)->scs;
            m_rtspClient->sendGetParameterCommand(*scs.session, pSessionPrivate->continueAfterGetParameter, "");
        }
    }

    if (isRtspSession())//只有拉了流才可以执行下面的逻辑
    {
        if (0.0f == pSessionPrivate->fLastRecvCheckTime)
        {
            return;
        }
        int TimeSpan = (int)(TimeSpanEx(pSessionPrivate->fLastRecvCheckTime)*1000);
        // 超时没收到视频，断线重连.这边断了以后需要立马关掉，不然会一直发掉线反馈。
        if ((pSessionPrivate->fLastRecvCheckTime >0.0f) &&
                (TimeSpan > pSessionPrivate->_nMaxFrameInterval))
        {
            pSessionPrivate->fLastRecvCheckTime = GetExactTime();
            onDisconnect(-1, __FUNCTION__);
        }
    }
}

bool CRTSPSession::isRtspSession()
{
    return pSessionPrivate->_rtspStream != NULL;
}

// bool CRTSPSession::onRtspData(char *pBuffer, int len)
// {
// 	EnterCriticalSection(&pRtspSessionPrivate->_lockCallback);
// 	if (pRtspSessionPrivate->_rtspDataCB)
// 	{
// 		pRtspSessionPrivate->_rtspDataCB((long)this,pBuffer,len,(int)pRtspSessionPrivate->_user);
// 	}
// 	LeaveCriticalSection(&pRtspSessionPrivate->_lockCallback);
//
// 	return true;
// }

bool CRTSPSession::onRtspData(char *pBuffer, int len)
{
    pSessionPrivate->fLastRecvCheckTime = GetExactTime();
    EnterCriticalSection(&pSessionPrivate->csCallBack);
    if (pSessionPrivate->_rtspStream)
    {
        pSessionPrivate->_rtspStream((long)this,pBuffer,(int)len, (int)pSessionPrivate->_user);
    }
    LeaveCriticalSection(&pSessionPrivate->csCallBack);

    return true;
}

void CRTSPSession::OnSDPNotify(_SDPAttrib *pSDP)
{
    //pSessionPrivate->fLastRecvCheckTime = GetExactTime();
    EnterCriticalSection(&pSessionPrivate->csCallBack);
    if (pSessionPrivate->_rtspSDPNotify)
    {
        pSessionPrivate->_rtspSDPNotify((long)this, pSDP, (int)pSessionPrivate->_user);
    }
    LeaveCriticalSection(&pSessionPrivate->csCallBack);
}
void CRTSPSession::onDisconnect(int nErrorCode,char *szProc)
{
    EnterCriticalSection(&pSessionPrivate->csCallBack);
    if (pSessionPrivate->_rtspDisconnect)
    {
        pSessionPrivate->_rtspDisconnect((long)this,nErrorCode,(int)pSessionPrivate->_user,szProc);
    }
    LeaveCriticalSection(&pSessionPrivate->csCallBack);
}

int CRTSPSession::openURL(UsageEnvironment& env, char const* rtspURL, int nHttpPort)
{
    m_rtspClient = ourRTSPClient::createNew(env, rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, "", nHttpPort);
    if (m_rtspClient == NULL)
    {
        env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
        return -1;
    }
    m_rtspClient->SetConnnectTimeout(nConnectTimeout);
    ((ourRTSPClient*)m_rtspClient)->session = this;

    ++CRTSPSessionPrivate::rtspClientCount;
    //((ourRTSPClient*)m_rtspClient)->m_nID = m_nID;
    m_rtspClient->sendDescribeCommand(CRTSPSessionPrivate::continueAfterDESCRIBE);
    return 0;
}

void CRTSPSession::Run()
{
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);
    if (openURL(*env, m_rtspUrl.c_str(),m_httpPort) == 0)
    {
        scheduler->session = this;
        //m_nStatus = 1;
        env->taskScheduler().doEventLoop(&eventLoopWatchVariable);

        //m_running = false;
        //eventLoopWatchVariable = 0;

        if (m_rtspClient)
        {
            CRTSPSessionPrivate::shutdownStream(m_rtspClient,0);
        }
        m_rtspClient = NULL;
    }

    env->reclaim();
    env = NULL;
    delete scheduler;
    scheduler = NULL;
    //m_nStatus = 2;
}
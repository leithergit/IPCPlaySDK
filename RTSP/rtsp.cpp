#include "rtsp.h"
#include "crtspsession.h"

#include <map>

class CritSec
{
public:
    CritSec()
    {
        InitializeCriticalSection(&m_CritSec);
    };
    ~CritSec()
    {
        DeleteCriticalSection(&m_CritSec);
    };
    void Lock()
    {
        EnterCriticalSection(&m_CritSec);
    };
    void UnLock()
    {
        LeaveCriticalSection(&m_CritSec);
    };

private:
    CRITICAL_SECTION m_CritSec;
};


CritSec g_mutex;
std::map<long,void*> g_sessions;


RTSP_EXPORT long rtsp_login(char* rtspURL,
                            const char* user,
                            const char* pass,
                            int httpPort,
                            PFRtspDisConnect cbDisconnectCallBack,
                            void* pUser,
                            int nConnectTimeout)
{
    CRTSPSession* pSesson = new CRTSPSession();

    if (pSesson->startRTSPClient(rtspURL, 0, nullptr, NULL, cbDisconnectCallBack, pUser, false, nConnectTimeout)<0)
    {
        delete pSesson;
        return 0;
    }

    g_mutex.Lock();
    g_sessions[(long)pSesson] = pSesson;
    g_mutex.UnLock();

    return (long)pSesson;
}

RTSP_EXPORT long rtsp_play(const char *rtspURL,
                           const char* user,
                           const char* pass,
                           int nNetType,
                           int httpPort,
                           void* cbSDPNotify,
                           void* cbDataCallBack,
                           void* cbDisconnectCallBack,
                           void* pUser,
                           int nConnectTimeout,
                           int nMaxFrameInterval)
{
    CRTSPSession* pSesson = new CRTSPSession();

    pSesson->setNetType(nNetType);
    if (pSesson->startRTSPClient(rtspURL,
                                 httpPort,
                                 (PFSDPCallBack)cbSDPNotify,
                                 (PFRtspDataCallBack)cbDataCallBack,
                                 (PFRtspDisConnect)cbDisconnectCallBack,
                                 pUser,
                                 false,
                                 nConnectTimeout,
                                 nMaxFrameInterval)<0)
    {
        delete pSesson;
        return 0;
    }

    g_mutex.Lock();
    g_sessions[(long)pSesson] = pSesson;
    g_mutex.UnLock();

    return (long)pSesson;
}

RTSP_EXPORT long rtsp_setCallback(long lPlayHandle,
                                  void* cbSDPNotify,
                                  void* cbDataCallBack,
                                  void* cbDisconnectCallBack,
                                  void* pUser)
{
    if (lPlayHandle)
    {
        CRTSPSession* pSesson = (CRTSPSession*)lPlayHandle;
    }
    return 0;
}

RTSP_EXPORT int	rtsp_stop(long lHandle)
{
    CRTSPSession* pSesson = NULL;

    g_mutex.Lock();
    std::map<long,void*>::iterator iter = g_sessions.find(lHandle);
    if(iter != g_sessions.end())
    {
        pSesson = (CRTSPSession*)iter->second;
        g_sessions.erase(iter);
    }
    g_mutex.UnLock();

    if(pSesson)
    {
        pSesson->stopRTSPClient();
        delete pSesson;
    }

    return 0;
}


//
//  "$Id: Thread.cpp 7005 2009-08-17 10:48:21Z wang_haifeng $"
//
//  Copyright (c)1992-2007, ZheJiang Dahua Technology Stock CO.LTD.
//  All Rights Reserved.
//
//	Description:
//	Revisions:		Year-Month-Day  SVN-Author  Modification
//

#include <assert.h>
#include "Thread.h"

#ifdef WIN32
#include <Windows.h>
#elif defined(__linux__)
#include <stdio.h>
#include <algorithm>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#define TSK_DEF_STACK_SIZE		16384
#elif defined(__TCS__)
#include <psos.h>
#include <pna.h>
#include <stdio.h>
#define TNR_OWNERID		0 //task notepad register 0 if filled with owner ID
#define TNR_PARAMETER	1 //task notepad register 2 if filled with function parameter
#define DEFAULT_STACK_SZ 0X8000
#else
#error "Unknown Platform"
#endif

#ifdef WIN32
int GeSystVersion()
{
    int nSysVirsion = 5;//4 98 5 xp 6 vista,7
    OSVERSIONINFOEX osvi;
    long nSize = sizeof(OSVERSIONINFOEX);
    ZeroMemory(&osvi,nSize);
    osvi.dwOSVersionInfoSize = nSize;
    if(GetVersionEx((OSVERSIONINFO*)&osvi))
    {
        nSysVirsion = osvi.dwMajorVersion;
    }

    return nSysVirsion;
}

struct ThreadParams
{
    ThreadBase* instance;
    bool joinable;
};

DWORD __stdcall ThreadFunc(void* params)
{
    ThreadParams* thread_params = static_cast<ThreadParams*>(params);
    ThreadBase* instance = thread_params->instance;

    //PlatformThreadHandle::Handle platform_handle;
    //BOOL did_dup = DuplicateHandle(GetCurrentProcess(),
    //	GetCurrentThread(),
    //	GetCurrentProcess(),
    //	&platform_handle,
    //	0,
    //	FALSE,
    //	DUPLICATE_SAME_ACCESS);

    delete thread_params;
    instance->ThreadMain();

    return NULL;
}
#elif defined(__linux__)
struct ThreadParams
{
    ThreadParams()
        : delegate(NULL),
          joinable(false),
          //priority(kThreadPriority_Normal),
          handle(NULL),
    { }

    ThreadBase* instance;
    bool joinable;
    //ThreadPriority priority;
    PlatformThreadHandle* handle;
};

void* ThreadFunc(void* params)
{
    //base::InitOnThread();
    ThreadParams* thread_params = static_cast<ThreadParams*>(params);

    ThreadBase* instance = thread_params->instance;
    if (!thread_params->joinable)
        base::ThreadRestrictions::SetSingletonAllowed(false);

    //if (thread_params->priority != kThreadPriority_Normal) {
    //	PlatformThread::SetThreadPriority(PlatformThread::CurrentHandle(),
    //		thread_params->priority);
    //}

    // Stash the id in the handle so the calling thread has a complete
    // handle, and unblock the parent thread.
    //*(thread_params->handle) = PlatformThreadHandle(pthread_self(),
    //	PlatformThread::CurrentId());

    instance->ThreadMain();

    //base::TerminateOnThread();
    return NULL;
}

#endif

bool CThread::CreateThread(size_t stack_size, bool joinable,
                           ThreadBase* instance,
                           PlatformThreadHandle* out_thread_handle,
                           ThreadPriority priority)
{

#ifdef WIN32
    unsigned int flags = 0;
    if (stack_size > 0 && GetVersion() >= 5)
    {
        flags = STACK_SIZE_PARAM_IS_A_RESERVATION;
    }
    else
    {
        stack_size = 0;
    }

    ThreadParams* params = new ThreadParams;
    params->instance = instance;
    params->joinable = out_thread_handle != NULL;

    void* thread_handle = ::CreateThread(
                              NULL, stack_size, ThreadFunc, params, flags, NULL);
    if (!thread_handle)
    {
        delete params;
        return false;
    }

    if (out_thread_handle)
        *out_thread_handle = PlatformThreadHandle(thread_handle);
    else
        CloseHandle(thread_handle);
    return true;
#elif defined(__linux__)
    bool success = false;
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);

    // Pthreads are joinable by default, so only specify the detached
    // attribute if the thread should be non-joinable.
    if (!joinable)
    {
        pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
    }

    // Get a better default if available.
    if (stack_size == 0)
        stack_size = base::GetDefaultThreadStackSize(attributes);

    if (stack_size > 0)
        pthread_attr_setstacksize(&attributes, stack_size);

    ThreadParams params;
    params.instance = instance;
    params.joinable = joinable;
    //params.priority = priority;
    params.handle = out_thread_handle;

    pthread_t handle;
    int err = pthread_create(&handle,
                             &attributes,
                             ThreadFunc,
                             &params);
    success = !err;
    if (!success)
    {
        // Value of |handle| is undefined if pthread_create fails.
        handle = 0;
        errno = err;
    }
    else
    {
        if (out_thread_handle)
            *out_thread_handle = PlatformThreadHandle(thread_handle);
    }

    pthread_attr_destroy(&attributes);

    return success;
#endif
}

void CThread::JoinThread(PlatformThreadHandle thread_handle)
{
#ifdef WIN32
    DWORD result = WaitForSingleObject(thread_handle.handle_, INFINITE);
    if (result != WAIT_OBJECT_0)
    {
        // Debug info for bug 127931.
        DWORD error = GetLastError();
    }

    CloseHandle(thread_handle.handle_);
#elif defined(__linux__)
    pthread_join(thread_handle.handle_, NULL)
#endif
}

CThread::CThread(const char* name_prefix)
    : thread_()
    , running_(false)
    , joined_(true)
{
    strcpy(name_prefix_,name_prefix);
}

CThread::CThread(const char* name_prefix, const Options& options)
    : options_(options)
    , thread_()
    , running_(false)
    , joined_(true)
{
    strcpy(name_prefix_,name_prefix);
}

CThread::~CThread()
{
    Join();
}

void CThread::Start()
{
    if (running_ == false)
    {
        running_ = true;
        bool success = CreateThread(options_.stack_size(), true,
                                    this, &thread_, kThreadPriority_Normal);
        joined_ = false;
    }
}
void CThread::Join()
{
    if (running_)
    {
        running_ = false;
        JoinThread(thread_);
        joined_ = true;
    }
}

void CThread::ThreadMain()
{
    while(running_)
    {
        Run();
    }
}

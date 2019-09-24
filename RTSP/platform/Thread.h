//
//  "$Id: Thread.h 1504 2008-09-18 03:49:01Z wang_haifeng $"
//
//  Copyright (c)1992-2007, ZheJiang Dahua Technology Stock CO.LTD.
//  All Rights Reserved.
//
//	Description:
//	Revisions:		Year-Month-Day  SVN-Author  Modification
//

#ifndef __INTER_SEMAPHORE_H__
#define __INTER_SEMAPHORE_H__

// Valid values for SetThreadPriority()
enum ThreadPriority
{
    kThreadPriority_Normal,
    // Suitable for low-latency, glitch-resistant audio.
    kThreadPriority_RealtimeAudio,
    // Suitable for threads which generate data for the display (at ~60Hz).
    kThreadPriority_Display,
    // Suitable for threads that shouldn't disrupt high priority work.
    kThreadPriority_Background
};

class PlatformThreadHandle
{
public:
#if defined(WIN32)
    typedef unsigned long PlatformThreadId;
    typedef void* Handle;
#elif defined(__linux__)
    typedef pid_t PlatformThreadId;
    typedef pthread_t Handle;
#endif

    PlatformThreadHandle()
        : handle_(0),
          id_(0)
    {
    }

    explicit PlatformThreadHandle(Handle handle)
        : handle_(handle),
          id_(0)
    {
    }

    PlatformThreadHandle(Handle handle,
                         PlatformThreadId id)
        : handle_(handle),
          id_(id)
    {
    }

    bool is_equal(const PlatformThreadHandle& other) const
    {
        return handle_ == other.handle_;
    }

    bool is_null() const
    {
        return !handle_;
    }

    Handle platform_handle() const
    {
        return handle_;
    }

private:
    friend class CThread;

    Handle handle_;
    PlatformThreadId id_;
};

class ThreadBase
{
public:
    virtual void ThreadMain() = 0;
};

class CThread : public ThreadBase
{
public:
    class Options
    {
    public:
        Options() : stack_size_(0) { }
        ~Options() { }

        void set_stack_size(size_t size)
        {
            stack_size_ = size;
        }
        size_t stack_size() const
        {
            return stack_size_;
        }
    private:
        size_t stack_size_;
    };

    CThread(const char*pName);
    CThread(const char*pName, const Options& options);
    virtual ~CThread();

    virtual void Start();
    virtual void Join();

    bool HasBeenStarted()
    {
        return running_;
    }

protected:
    virtual void Run() = 0;

    static bool CreateThread(size_t stack_size, bool joinable,
                             ThreadBase* instance,
                             PlatformThreadHandle* out_thread_handle,
                             ThreadPriority priority);

    static void JoinThread(PlatformThreadHandle thread_handle);
private:
    virtual void ThreadMain();
private:
    char name_prefix_[128];
    const Options options_;
    PlatformThreadHandle thread_;  // PlatformThread handle, invalid after Join!
protected:
    bool running_;
    bool joined_;                  // True if Join has been called.
};


#endif

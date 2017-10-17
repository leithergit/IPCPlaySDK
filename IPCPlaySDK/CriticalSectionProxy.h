#pragma once
#include <windows.h>

using namespace std;

#ifdef _STD_SMARTPTR
#include <memory>
using namespace std::tr1;
#else
#include <boost/shared_ptr.hpp>
using namespace boost;
#endif
class CCriticalSectionProxy
{
public:
	CCriticalSectionProxy()
	{
		InitializeCriticalSection(&m_cs);
	}
	~CCriticalSectionProxy()
	{
		DeleteCriticalSection(&m_cs);
	}
	inline BOOL TryLock()
	{
		return TryEnterCriticalSection(&m_cs);
	}
	inline void Lock()
	{
		EnterCriticalSection(&m_cs);
	}
	void Unlock()
	{
		LeaveCriticalSection(&m_cs);
	}
	inline CRITICAL_SECTION *Get()
	{
		return &m_cs;
	}

protected:
	CRITICAL_SECTION	m_cs;
};

typedef shared_ptr<CCriticalSectionProxy>CCriticalSectionProxyPtr;

#pragma once
#include "stdafx.h"
#include <new>
#include <assert.h>
using namespace std;

#ifdef _STD_SMARTPTR
#include <memory>
//using namespace std::tr1;
#else
#include <boost/shared_ptr.hpp>
using namespace boost;
#endif

#include <list>
#include <algorithm>
#include <windows.h>
#include <winsock2.h>
#include <process.h>
#include <Shlwapi.h>
#include <MMSystem.h>
#include "Runlog.h"
#include "Utility.h"
//#include <VersionHelpers.h> // Windows SDK 8.1 ≤≈”–‡∏
#pragma comment(lib, "winmm.lib")
#ifdef _DEBUG
#define _New	new
#else
#define _New	new (std::nothrow)
#endif


using namespace std;
//using namespace std::tr1;
#pragma comment(lib,"ws2_32")
#pragma warning (disable:4018)

#define _IPCPlaySDKMutex	_T("Global\\{7E046525-8DEA-4263-957A-AD6C6FCEE19B}")
#define _SharedMemoryName	_T("Global\\{1BE0CD67-A6F9-4BE1-AD95-0784BED9F73C}")



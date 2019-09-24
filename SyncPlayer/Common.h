#pragma once
#include "stdafx.h"
#include <new>
#include <assert.h>
using namespace std;

#ifdef _STD_SMARTPTR
#include <memory>
using namespace std::tr1;
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
using namespace std::tr1;
#pragma comment(lib,"ws2_32")
#pragma warning (disable:4018)
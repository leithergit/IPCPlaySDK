#pragma once
#include "stdafx.h"
#include <new>
#include <assert.h>
#include <list>
#include <memory>
#include <algorithm>
#include <windows.h>
#include <winsock2.h>
#include <process.h>
#include <Shlwapi.h>
#include <MMSystem.h>
#include "Runlog.h"
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
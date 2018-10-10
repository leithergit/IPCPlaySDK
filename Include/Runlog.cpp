// Runlog.cpp: implementation of the CRunlog class.
//
//////////////////////////////////////////////////////////////////////

//#include "stdafx.h"

#include "Runlog.h"
#include <TCHAR.H>
#include <stdio.h>
#include <time.h>

#include <Shlwapi.h>
#include "Utility.h"
#include "TimeUtility.h"

using namespace std;


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
//#define new DEBUG_NEW
#endif

#pragma comment(lib,"Shlwapi.lib")
#if !defined(_countof)
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

#define _DatetimeLen	8
#define _ExtNameLen		4
#define _LogBuffLength	1024*4
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#pragma warning(disable:4996 4101) 
volatile bool g_bEnableRunlog = true;
/*CRunlogHelperPtr g_pRunlogHelperPtr = shared_ptr<CRunlogHelper>(new CRunlogHelper());*/

void EnableRunlog(bool bEnable)
{
	g_bEnableRunlog = bEnable;
}

void CRunlogA::CheckDateTime()
{
	CHAR	szDataTime[32] = {0};
	SYSTEMTIME tm;
	GetLocalTime(&tm);
	sprintf(szDataTime,("%04d%02d%02d"),tm.wYear,tm.wMonth,tm.wDay);
	__try
	{
		if ((tm.wYear == m_systimeCreate.wYear) && (tm.wMonth == m_systimeCreate.wMonth) && (tm.wDay == m_systimeCreate.wDay))
			__leave;
		//重新创建文件
			int nLen = strlen(m_szFileName) - _DatetimeLen - _ExtNameLen;	// 取得原始输入的文件名
			strcpy(&m_szFileName[nLen],szDataTime);		
			strcat(m_szFileName,(".log"));
			::EnterCriticalSection(&m_csFile);
			CloseHandle(m_hLogFile);
			m_hLogFile = NULL;
			m_hLogFile = CreateFileA(m_szFileName,
				GENERIC_WRITE|GENERIC_READ,
				FILE_SHARE_READ|FILE_SHARE_WRITE,
				NULL,
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_ARCHIVE,
				NULL);
			::LeaveCriticalSection(&m_csFile);
			if (m_hLogFile == NULL) 
				m_bCanlog = false;
			else
				GetLocalTime(&m_systimeCreate);
	}
	__finally
	{

	}
}
void CRunlogW::CheckDateTime()
{
	WCHAR	szDataTime[32] = { 0 };
	SYSTEMTIME tm;
	GetLocalTime(&tm);
	swprintf_s(szDataTime, 32, L"%04d%02d%02d", tm.wYear, tm.wMonth, tm.wDay);
	__try
	{
		if ((tm.wYear == m_systimeCreate.wYear) && (tm.wMonth == m_systimeCreate.wMonth) && (tm.wDay == m_systimeCreate.wDay))
			__leave;
		//重新创建文件
		int nLen = wcslen(m_szFileName) - _DatetimeLen - _ExtNameLen;	// 取得原始输入的文件名
		wcscpy(&m_szFileName[nLen], szDataTime);
		wcscat(m_szFileName, L".log");
		::EnterCriticalSection(&m_csFile);
		CloseHandle(m_hLogFile);
		m_hLogFile = NULL;
		m_hLogFile = CreateFileW(m_szFileName,
			GENERIC_WRITE | GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_ARCHIVE,
			NULL);
		::LeaveCriticalSection(&m_csFile);
		if (m_hLogFile == NULL)
			m_bCanlog = false;
		else
			GetLocalTime(&m_systimeCreate);
	}
	__finally
	{

	}
}
CRunlogA::CRunlogA()
{
	if (!g_bEnableRunlog)
	{
		return ;
	}
	m_hLogFile = NULL;
	//TCHAR szFileName[255] = {0};	
	CHAR szApppath[MAX_PATH] = {0};
	m_bCanlog = true;	
	SYSTEMTIME tm;
	try
	{
		GetLocalTime(&tm);		
		GetAppPathA(szApppath,MAX_PATH);
		sprintf(m_szFileName,("%s\\log\\%04d%02d%02d.log"),szApppath,tm.wYear,tm.wMonth,tm.wDay);
		strcat(szApppath,("\\log"));
		if (!PathFileExistsA(szApppath))
		{
			CreateDirectoryA(szApppath, NULL);
		}		
		
		m_hLogFile = CreateFileA(m_szFileName,
								GENERIC_WRITE|GENERIC_READ,
								FILE_SHARE_READ|FILE_SHARE_WRITE,
								NULL,
								OPEN_ALWAYS,
								FILE_ATTRIBUTE_ARCHIVE,
								NULL);
		if (m_hLogFile == NULL) 
			m_bCanlog = false;
		else					
			GetLocalTime(&m_systimeCreate);
		m_pLogBuffer = shared_ptr<CHAR>(new CHAR[_LogBuffLength]);
/*		g_pRunlogHelperPtr->Add(this);*/
		
	}
	catch (std::exception &e)
	{
		m_bCanlog = false;
		m_hLogFile = NULL;
	}
}
CRunlogA::CRunlogA(LPCSTR lpszFileName)
{
	if (!g_bEnableRunlog)
	{
		return ;
	}
	m_hLogFile = NULL;
	m_bCanlog = true;
	CHAR szApppath[MAX_PATH] = {0};	
	SYSTEMTIME tm;	
	try
	{
		GetLocalTime(&tm);
		GetAppPathA(szApppath,MAX_PATH);
		//_tcscpy(szApppath,_T("d:/"));
		if (strstr(lpszFileName,".log"))
		{
			CHAR szTempFileName[MAX_PATH] = {0};
			memcpy(szTempFileName,lpszFileName,(strlen(lpszFileName) - 4)*sizeof(TCHAR));
			sprintf(m_szFileName,"%s\\log\\%s%02d%02d%02d.log",szApppath,szTempFileName,tm.wYear,tm.wMonth,tm.wDay);
		}
		else
			sprintf(m_szFileName,"%s\\log\\%s%02d%02d%02d.log",szApppath,lpszFileName,tm.wYear,tm.wMonth,tm.wDay);
		strcat(szApppath,("\\log"));
		if (!PathFileExistsA(szApppath))
		{
			CreateDirectoryA(szApppath, NULL);
		}
		m_hLogFile = CreateFileA(m_szFileName,
								GENERIC_WRITE|GENERIC_READ,
								FILE_SHARE_READ|FILE_SHARE_WRITE,
								NULL,
								OPEN_ALWAYS,
								FILE_ATTRIBUTE_ARCHIVE,
								NULL);
		if (m_hLogFile == NULL) 
			m_bCanlog = false;
		else
			GetLocalTime(&m_systimeCreate);
		m_pLogBuffer = shared_ptr<CHAR>(new CHAR[_LogBuffLength]);
/*		g_pRunlogHelperPtr->Add(this);*/
		
	}
	catch (std::exception &e)
	{
		m_bCanlog = false;
		m_hLogFile = NULL;
	}
}
CRunlogW::CRunlogW()
{
	if (!g_bEnableRunlog)
	{
		return;
	}
	m_hLogFile = NULL;
	WCHAR szApppath[MAX_PATH] = { 0 };
	m_bCanlog = true;
	SYSTEMTIME tm;
	try
	{
		GetLocalTime(&tm);
		GetAppPathW(szApppath, MAX_PATH);
		swprintf(m_szFileName,  L"%s\\log\\%04d%02d%02d.log", szApppath, tm.wYear, tm.wMonth, tm.wDay);
		wcscat(szApppath, L"\\log");
		if (!PathFileExistsW(szApppath))
		{
			CreateDirectoryW(szApppath, NULL);
		}

		m_hLogFile = CreateFileW(m_szFileName,
			GENERIC_WRITE | GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_ARCHIVE,
			NULL);
		if (m_hLogFile == NULL)
			m_bCanlog = false;
		else
			GetLocalTime(&m_systimeCreate);
		m_pLogBuffer = shared_ptr<WCHAR>(new WCHAR[_LogBuffLength]);
/*		g_pRunlogHelperPtr->Add(this);*/
	}
	catch (std::exception &e)
	{
		m_bCanlog = false;
		m_hLogFile = NULL;
	}
}
CRunlogW::CRunlogW(LPCWSTR lpszFileName)
{
	if (!g_bEnableRunlog)
	{
		return;
	}
	m_hLogFile = NULL;
	m_bCanlog = true;
	WCHAR szApppath[MAX_PATH] = { 0 };
	SYSTEMTIME tm;
	try
	{
		GetLocalTime(&tm);
		GetAppPathW(szApppath, MAX_PATH);
		if (wcsstr(lpszFileName, L".log"))
		{
			WCHAR szTempFileName[MAX_PATH] = { 0 };
			memcpy(szTempFileName, lpszFileName, (wcslen(lpszFileName) - 4)*sizeof(WCHAR));
			swprintf(m_szFileName,  L"%s\\log\\%s%02d%02d%02d.log", szApppath, szTempFileName, tm.wYear, tm.wMonth, tm.wDay);
		}
		else
			swprintf(m_szFileName,  L"%s\\log\\%s%02d%02d%02d.log", szApppath, lpszFileName, tm.wYear, tm.wMonth, tm.wDay);
		wcscat(szApppath, L"\\log");
		if (!PathFileExistsW(szApppath))
		{
			CreateDirectoryW(szApppath, NULL);
		}
		m_hLogFile = CreateFileW(m_szFileName,
			GENERIC_WRITE | GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_ARCHIVE,
			NULL);
		if (m_hLogFile == NULL)
			m_bCanlog = false;
		else
			GetLocalTime(&m_systimeCreate);
		m_pLogBuffer = shared_ptr<WCHAR>(new WCHAR[_LogBuffLength]);
/*		g_pRunlogHelperPtr->Add(this);*/

	}
	catch (std::exception &e)
	{
		m_bCanlog = false;
		m_hLogFile = NULL;
	}
}

void CRunlogA::RunlogBin(LPCSTR szTitle,byte *pBinBuff,int nLen,CHAR chSeperator)
{
	if (!g_bEnableRunlog)
	{
		return ;
	}
	if (NULL == szTitle || NULL == pBinBuff || 0 == nLen)
		return;
	if (m_bCanlog)
	{	
		try
		{
			//检查当前的文件是否当前日期产生
			CHAR	szDataTime[32] = {0};
			::EnterCriticalSection(&m_csRunlogSection);
			CHAR	*szBuffer = m_pLogBuffer.get();
			StringFromSystemTimeA(szBuffer,_LogBuffLength);	
			int nDatetimeLen = strlen((char *)szBuffer);
			szBuffer[nDatetimeLen++] = ' ';
			CHAR szTitleA[256] = {0};
			CHAR szSeperatorA[4] = {0};
			CHAR chSeperator1;
			strcat((char *)szBuffer,(char *)szTitle);
			chSeperator1 = chSeperator;
			int nBuffLen = strlen((char *)szBuffer);
			Hex2AscStringA(pBinBuff,nLen,(CHAR *)&szBuffer[nBuffLen],_LogBuffLength - nBuffLen,chSeperator1);
// 			DWORD dwWritten = 0;
// 			SetFilePointer(m_hLogFile,0,NULL,FILE_END);
// 			WriteFile(m_hLogFile,szBuffer,strlen((char *)szBuffer),&dwWritten,NULL);
			m_listBuffer.push_back(string(szBuffer));
			m_listBuffer.push_back(string("\r\n"));

// 			szBuffer[0] = 0x0D;
// 			szBuffer[1] = 0x0A;
// 			szBuffer[2] = 00;
// 			WriteFile(m_hLogFile,szBuffer,2,&dwWritten,NULL);
// 			FlushFileBuffers(m_hLogFile);
			::LeaveCriticalSection(&m_csRunlogSection);

		}
		catch (std::exception &e)
		{
			if (m_hLogFile != NULL)
			{
				CloseHandle(m_hLogFile);
				m_hLogFile = NULL;
			}		
			m_bCanlog = false;
		}
	}
}
void CRunlogA::Runlog(LPCSTR pFormat, ...)
{
	if (!g_bEnableRunlog)
	{
		return ;
	}
	if (m_bCanlog)
	{
		try
		{
			CheckDateTime();
			va_list args;
			va_start(args, pFormat);
			::EnterCriticalSection(&m_csRunlogSection);
			CHAR *szBuffer = m_pLogBuffer.get();
			int nDatetimeLen = 0;			
			StringFromSystemTimeA(szBuffer,_LogBuffLength);
			nDatetimeLen = strlen(szBuffer);
			szBuffer[nDatetimeLen++] = ' ';
			vsnprintf((CHAR *)&szBuffer[nDatetimeLen], _LogBuffLength - nDatetimeLen, pFormat, args);
			//DWORD dwWritten = 0;	
			//SetFilePointer(m_hLogFile,0,NULL,FILE_END);			
			//WriteFile(m_hLogFile,szBuffer,strlen(szBuffer),&dwWritten,NULL);
			//FlushFileBuffers(m_hLogFile);
			m_listBuffer.push_back(string(szBuffer));
			::LeaveCriticalSection(&m_csRunlogSection);
			va_end(args);
		}
		catch (std::exception &e)
		{
			if (m_hLogFile != NULL)
			{
				CloseHandle(m_hLogFile);
				m_hLogFile = NULL;
			}		
			m_bCanlog = false;
		}
	}
}
void CRunlogA::RunlogHuge(byte *szHugeData,int nDataLen,LPCSTR pFormat,...)
{
	if (!g_bEnableRunlog)
	{
		return ;
	}
	if (m_bCanlog)
	{
		try
		{
			//检查当前的文件是否当前日期产生
			CheckDateTime();	
			va_list args;
			va_start(args, pFormat);
			::EnterCriticalSection(&m_csRunlogSection);
			CHAR *szBuffer = m_pLogBuffer.get();	
			int nDatetimeLen = 0;
			StringFromSystemTimeA(szBuffer,_LogBuffLength);
			nDatetimeLen = strlen(szBuffer);
			szBuffer[nDatetimeLen ++] = ' ';
			vsnprintf((CHAR *)&szBuffer[nDatetimeLen], _LogBuffLength - nDatetimeLen , pFormat, args);
			m_listBuffer.push_back(string(szBuffer));
			m_listBuffer.push_back(string((char *)szHugeData,nDataLen));
//			DWORD dwWritten = 0;					
// 			SetFilePointer(m_hLogFile,0,NULL,FILE_END);
// 			BOOL bWrite = WriteFile(m_hLogFile,szBuffer,strlen(szBuffer),&dwWritten,NULL);
// 			//SetFilePointer(m_hLogFile,0,NULL,FILE_END);
// 			bWrite = WriteFile(m_hLogFile,szHugeData,nDataLen,&dwWritten,NULL);
// 			FlushFileBuffers(m_hLogFile);
			::LeaveCriticalSection(&m_csRunlogSection);			
			va_end(args);
		}
		catch (std::exception &e)
		{
			if (m_hLogFile != NULL)
			{
				CloseHandle(m_hLogFile);
				m_hLogFile = NULL;
			}		
			m_bCanlog = false;
		}
	}
}
void CRunlogA::Runlogv(LPCSTR pFormat, va_list args)
{
	if (!g_bEnableRunlog)
	{
		return ;
	}
	if (m_bCanlog)
	{
		try
		{
			//检查当前的文件是否当前日期产生
			CheckDateTime();
			::EnterCriticalSection(&m_csRunlogSection);
			CHAR *szBuffer = m_pLogBuffer.get();
			int nDatetimeLen = 0;
			StringFromSystemTimeA(szBuffer,_LogBuffLength);
			nDatetimeLen = strlen(szBuffer);
			szBuffer[nDatetimeLen++] = ' ';
			vsnprintf((CHAR *)&szBuffer[nDatetimeLen], _LogBuffLength - nDatetimeLen, pFormat, args);
			DWORD dwWritten = 0;	
			m_listBuffer.push_back(string(szBuffer));
// 			SetFilePointer(m_hLogFile,0,NULL,FILE_END);
// 			WriteFile(m_hLogFile,szBuffer,strlen(szBuffer),&dwWritten,NULL);
// 			FlushFileBuffers(m_hLogFile);
			::LeaveCriticalSection(&m_csRunlogSection);			
		}
		catch (std::exception &e)
		{
			if (m_hLogFile != NULL)
			{
				CloseHandle(m_hLogFile);
				m_hLogFile = NULL;
			}		
			m_bCanlog = false;
		}
	}
}
void CRunlogA::RunlogHugev(LPCSTR szHugeText,const CHAR *pFormat, va_list args)
{
	if (!g_bEnableRunlog)
	{
		return ;
	}
	if (m_bCanlog)
	{
		try
		{
			CheckDateTime();
			::EnterCriticalSection(&m_csRunlogSection);
			CHAR *szBuffer = m_pLogBuffer.get();
			int nDatetimeLen = 0;
			int nHugeTextLen = 0;
			byte *pHugeTextA = NULL;

			StringFromSystemTimeA(szBuffer,_LogBuffLength);
			nDatetimeLen = strlen(szBuffer);
			szBuffer[nDatetimeLen++] = ' ';
			vsnprintf((CHAR *)&szBuffer[nDatetimeLen], _LogBuffLength - nDatetimeLen, pFormat, args);
			DWORD dwWritten = 0;			
			
			m_listBuffer.push_back(string(szBuffer));
			m_listBuffer.push_back(string(szHugeText,nHugeTextLen));
// 			SetFilePointer(m_hLogFile,0,NULL,FILE_END);
// 			WriteFile(m_hLogFile,szBuffer,strlen((LPSTR)szBuffer),&dwWritten,NULL);
// 			SetFilePointer(m_hLogFile,0,NULL,FILE_END);
// 			WriteFile(m_hLogFile,szHugeText,nHugeTextLen,&dwWritten,NULL);			
// 			FlushFileBuffers(m_hLogFile);
			::LeaveCriticalSection(&m_csRunlogSection);
		}
		catch (std::exception &e)
		{
			if (m_hLogFile != NULL)
			{
				CloseHandle(m_hLogFile);
				m_hLogFile = NULL;
			}		
			m_bCanlog = false;
		}
	}
}

void CRunlogW::Runlog(LPCWSTR pFormat, ...)
{
	if (!g_bEnableRunlog)
	{
		return ;
	}
	if (m_bCanlog)
	{
		try
		{
			CheckDateTime();
			va_list args;
			va_start(args, pFormat);			
			int nDatetimeLen = 0;
			::EnterCriticalSection(&m_csRunlogSection);
			WCHAR *szBufferW = m_pLogBuffer.get();
			StringFromSystemTimeW(szBufferW,_LogBuffLength);
			nDatetimeLen = wcslen(szBufferW);
			szBufferW[nDatetimeLen++] = _T(' ');
			_vsnwprintf((WCHAR *)&szBufferW[nDatetimeLen], _LogBuffLength - nDatetimeLen, pFormat, args);
			DWORD dwWritten = 0;
			int nBufferLen = 0;
			shared_ptr<char> pBufferA = W2AString(szBufferW,nBufferLen);	
			m_listBuffer.push_back(string(pBufferA.get()));
// 			SetFilePointer(m_hLogFile,0,NULL,FILE_END);			
// 			WriteFile(m_hLogFile,pBufferA.get(),nBufferLen,&dwWritten,NULL);
// 			FlushFileBuffers(m_hLogFile);
			::LeaveCriticalSection(&m_csRunlogSection);
			va_end(args);
		}
		catch (std::exception &e)
		{
			if (m_hLogFile != NULL)
			{
				CloseHandle(m_hLogFile);
				m_hLogFile = NULL;
			}		
			m_bCanlog = false;
		}
	}
}
void CRunlogW::RunlogBin(LPCWSTR szTitle,byte *pBinBuff,int nLen,WCHAR chSeperator)
{
	if (!g_bEnableRunlog)
	{
		return ;
	}
	if (NULL == szTitle || NULL == pBinBuff || 0 == nLen)
		return;
	if (m_bCanlog)
	{	
		try
		{
			//检查当前的文件是否当前日期产生
			WCHAR	szDataTime[32] = {0};
			::EnterCriticalSection(&m_csRunlogSection);
			WCHAR	*szBuffer = m_pLogBuffer.get();
			StringFromSystemTimeW(szBuffer,_LogBuffLength);	
			int nDatetimeLen = wcslen((WCHAR*)szBuffer);
			szBuffer[nDatetimeLen++] = L' ';
			WCHAR szTitle[256] = {0};
			WCHAR szSeperator[4] = {0};			
			wcscat((WCHAR *)szBuffer,(WCHAR*)szTitle);		
			int nBuffLen = wcslen((WCHAR*)szBuffer);
			Hex2AscStringW(pBinBuff,nLen,(WCHAR *)&szBuffer[nBuffLen],_LogBuffLength - nBuffLen,chSeperator);	
			shared_ptr<char> pBufferA = W2AString(szBuffer,nBuffLen);
			m_listBuffer.push_back(string(pBufferA.get()));
			m_listBuffer.push_back(string("\r\n"));
// 			DWORD dwWritten = 0;			
// 			SetFilePointer(m_hLogFile,0,NULL,FILE_END);
// 			WriteFile(m_hLogFile,pBufferA.get(),nBuffLen,&dwWritten,NULL);
// 			char *szLine = "\r\n";		// 回车换行			
//  		WriteFile(m_hLogFile,szLine,2,&dwWritten,NULL);
//  		FlushFileBuffers(m_hLogFile);
 			::LeaveCriticalSection(&m_csRunlogSection);
		}
		catch (std::exception &e)
		{
			if (m_hLogFile != NULL)
			{
				CloseHandle(m_hLogFile);
				m_hLogFile = NULL;
			}		
			m_bCanlog = false;
		}
	}
}
void CRunlogW::RunlogHuge(byte *szHugeData,int nDataLen,LPCWSTR pFormat,...)
{
	if (!g_bEnableRunlog)
	{
		return ;
	}
	if (m_bCanlog)
	{
		try
		{
			//检查当前的文件是否当前日期产生
			CheckDateTime();	
			va_list args;
			va_start(args, pFormat);	
			::EnterCriticalSection(&m_csRunlogSection);
			WCHAR *szBuffer = m_pLogBuffer.get();
			StringFromSystemTimeW(szBuffer,_LogBuffLength);
			int nDatetimeLen = wcslen(szBuffer);
			szBuffer[nDatetimeLen ++] = L' ';
			szBuffer[nDatetimeLen] = L'\0';
			_vsnwprintf((WCHAR *)&szBuffer[nDatetimeLen], _LogBuffLength - nDatetimeLen, pFormat, args);
			int nBufferLen = 0;
			shared_ptr<char> pBufferA = W2AString(szBuffer,nBufferLen);
			m_listBuffer.push_back(string(pBufferA.get()));
			m_listBuffer.push_back(string((char *)szHugeData,nDataLen));
// 			DWORD dwWritten = 0;			
// 			SetFilePointer(m_hLogFile,0,NULL,FILE_END);
// 			BOOL bWrite = WriteFile(m_hLogFile,pBufferA.get(),nBufferLen,&dwWritten,NULL);
// 			SetFilePointer(m_hLogFile,0,NULL,FILE_END);
// 			bWrite = WriteFile(m_hLogFile,szHugeData,nDataLen,&dwWritten,NULL);
// 			FlushFileBuffers(m_hLogFile);
			::LeaveCriticalSection(&m_csRunlogSection);			
			va_end(args);
		}
		catch (std::exception &e)
		{
			if (m_hLogFile != NULL)
			{
				CloseHandle(m_hLogFile);
				m_hLogFile = NULL;
			}		
			m_bCanlog = false;
		}
	}
}
void CRunlogW::Runlogv(LPCWSTR pFormat, va_list args)
{
	if (!g_bEnableRunlog)
	{
		return ;
	}
	if (m_bCanlog)
	{
		try
		{
			//检查当前的文件是否当前日期产生
			CheckDateTime();			
			int nDatetimeLen = 0;
			::EnterCriticalSection(&m_csRunlogSection);			
			WCHAR *szBuffer = m_pLogBuffer.get();
			StringFromSystemTimeW(szBuffer,_LogBuffLength);
			nDatetimeLen = wcslen(szBuffer);
			szBuffer[nDatetimeLen++] = _T(' ');
			_vsnwprintf((WCHAR *)&szBuffer[nDatetimeLen], _LogBuffLength - nDatetimeLen, pFormat, args);
			int nBufferLen = 0;
			shared_ptr<char> pBufferA = W2AString(szBuffer,nBufferLen);
			m_listBuffer.push_back(string(pBufferA.get()));
// 			SetFilePointer(m_hLogFile,0,NULL,FILE_END);
// 			DWORD dwWritten = 0;	
// 			WriteFile(m_hLogFile,pBufferA.get(),nBufferLen,&dwWritten,NULL);
// 			FlushFileBuffers(m_hLogFile);
			::LeaveCriticalSection(&m_csRunlogSection);			
		}
		catch (std::exception &e)
		{
			if (m_hLogFile != NULL)
			{
				CloseHandle(m_hLogFile);
				m_hLogFile = NULL;
			}		
			m_bCanlog = false;
		}
	}
}
void CRunlogW::RunlogHugev(LPCWSTR szHugeText,const WCHAR *pFormat, va_list args)
{
	if (!g_bEnableRunlog)
	{
		return ;
	}
	if (m_bCanlog)
	{
		try
		{
			CheckDateTime();						
			int nDatetimeLen = 0;
			::EnterCriticalSection(&m_csRunlogSection);
			WCHAR *szBuffer = m_pLogBuffer.get();
			StringFromSystemTimeW(szBuffer,_LogBuffLength);
			nDatetimeLen = wcslen(szBuffer);
			szBuffer[nDatetimeLen++] = ' ';
			_vsnwprintf((WCHAR *)&szBuffer[nDatetimeLen], _LogBuffLength - nDatetimeLen, pFormat, args);

			int nBufferLen = 0;
			shared_ptr<char> pBufferA = W2AString(szBuffer,nBufferLen);
			int nHugeTextLen = 0;
			shared_ptr<char> pHugeTextA = W2AString(szHugeText,nHugeTextLen);			
			m_listBuffer.push_back(string(pBufferA.get()));
			m_listBuffer.push_back(string(pHugeTextA.get()));
// 			DWORD dwWritten = 0;			
// 			SetFilePointer(m_hLogFile,0,NULL,FILE_END);
// 			WriteFile(m_hLogFile,pBufferA.get(),strlen((LPSTR)szBuffer),&dwWritten,NULL);
// 			SetFilePointer(m_hLogFile,0,NULL,FILE_END);
// 			WriteFile(m_hLogFile,pHugeTextA.get(),strlen((LPCSTR)pHugeTextA.get()),&dwWritten,NULL);			
// 			FlushFileBuffers(m_hLogFile);
			::LeaveCriticalSection(&m_csRunlogSection);

		}
		catch (std::exception &e)
		{
			if (m_hLogFile != NULL)
			{
				CloseHandle(m_hLogFile);
				m_hLogFile = NULL;
			}		
		}
	}
}
CRunlogA::~CRunlogA()
{
/*	g_pRunlogHelperPtr->Remove(this);*/
	
	if (!g_bEnableRunlog)
	{
		return ;
	}
	
	if (m_bCanlog)
	{
		Flush();
		::EnterCriticalSection(&m_csRunlogSection);
		FlushFileBuffers(m_hLogFile);
		if (CloseHandle(m_hLogFile))
		{	
			m_hLogFile = NULL;
			m_bCanlog = false;
		}
		::LeaveCriticalSection(&m_csRunlogSection);
	}
}
CRunlogW::~CRunlogW()
{
/*	g_pRunlogHelperPtr->Remove(this);*/
	TraceMsgW(L"%s\n", __FUNCTIONW__);
	if (!g_bEnableRunlog)
	{
		return;
	}
	
	if (m_bCanlog)
	{
		Flush();
// 		__try
// 		{
		::EnterCriticalSection(&m_csRunlogSection);
		FlushFileBuffers(m_hLogFile);
		if (CloseHandle(m_hLogFile))
		{
			m_hLogFile = NULL;
			m_bCanlog = false;
		}
		::LeaveCriticalSection(&m_csRunlogSection);
// 		}
// 		__except (1)
// 		{
// 			m_hLogFile = NULL;
// 			m_bCanlog = false;
// 		}
	}
}

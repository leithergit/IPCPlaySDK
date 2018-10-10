#pragma once
#ifndef QSOCKETSERVER_H
#define QSOCKETSERVER_H

#include <WINSOCK2.H>  
#include "Utility.h"
#include "TimeUtility.h"
#include "Runlog.h"
#include <Ws2tcpip.h>
#include <process.h>

#define WSA_VERSION MAKEWORD(2,2)
#define MAX_BUFFER_LEN		8192  

#ifdef	_DEBUGTEST
#define		__MaxClients	4			// 每个线程的最大连接数
#else
#define		__MaxClients	64			// 每个线程的最大连接数
#endif
#define		__MaxThread		8
using namespace std;


struct ThreadParam
{
	void*			pParam;
	int				nThreadIndex;
};
struct CSockClient
{

public:
	SOCKET		s;
	sockaddr_in ClientAddr;
	WSAEVENT	hSocketEvent;
	double		tLastAction;		// the time in million second for last time the socket has a  action 
	char		*pRecvBuffer;	
	int			nBufferLength;
	int			nRecvBufferSize;
	SOCKADDR_IN ServerAddr;
	UINT		nRecvTimeout;
	UINT		nSendTimeout;
	CRunlog*	pRunlog;
	CSockClient()
	{
		pRecvBuffer = NULL;
		nBufferLength = 0;
		ZeroMemory(this, sizeof(CSockClient)); 	
		pRecvBuffer = new char[MAX_BUFFER_LEN];
		if (pRecvBuffer == NULL)
		{
			nRecvBufferSize = 0;
			TraceMsg(_T("CSockClient%d.\n"),__FUNCTION__);
		}
		ZeroMemory(pRecvBuffer,MAX_BUFFER_LEN);
		nRecvBufferSize = MAX_BUFFER_LEN;
		nRecvTimeout = 5000;
		nSendTimeout = 5000;
		s = INVALID_SOCKET;
	}

	CSockClient(SOCKET s,sockaddr_in AddrIn)
	{
		pRecvBuffer = NULL;
		nBufferLength = 0;
		ZeroMemory(this, sizeof(CSockClient)); 	
		pRecvBuffer = new char[MAX_BUFFER_LEN];
		if (pRecvBuffer == NULL)
		{
			nRecvBufferSize = 0;
			TraceMsg(_T("CSockClient%d.\n"),__FUNCTION__);
		}
		ZeroMemory(pRecvBuffer,MAX_BUFFER_LEN);
		nRecvBufferSize = MAX_BUFFER_LEN;
		nRecvTimeout = 5000;
		nSendTimeout = 5000;
		memcpy(&ClientAddr,&AddrIn,sizeof(sockaddr_in));
		Attatch(s);
		tLastAction = GetExactTime();
	}
	bool Attatch(SOCKET s)
	{
		this->s = s;
		int nNonBlock = 1;
		DWORD dwErrorCode		= ioctlsocket(this->s, FIONBIO, (unsigned long*)&nNonBlock);
		if(dwErrorCode != 0)
			return false;

		hSocketEvent = WSACreateEvent();
		if (!hSocketEvent)
		{
			dwErrorCode = GetLastError();
			return false;
		}
		if (SOCKET_ERROR == WSAEventSelect(s, hSocketEvent, FD_READ|FD_WRITE|FD_CONNECT|FD_CLOSE))
			return false;
		return true;
	}

	~CSockClient()
	{
		if (pRecvBuffer)
		{
			delete []pRecvBuffer;
			pRecvBuffer = NULL;
		}
		if (hSocketEvent)
		{
			WSACloseEvent(hSocketEvent);
			hSocketEvent = NULL;
		}
		if (s != INVALID_SOCKET)
		{
			DisConnect(s);
			s = INVALID_SOCKET;
		}
#ifdef _DEBUG
		ZeroMemory(this, sizeof(CSockClient)); 	
#endif
	}

	int ConnectServer(LPCTSTR szServerIP, WORD nPort, int nNetTimeout)
	{
		bool bConnected = true;
		SOCKET s = INVALID_SOCKET;
		DWORD dwError = 0;
		__try
		{
			s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (s == INVALID_SOCKET)
			{
				bConnected = false;
				__leave;
			}

			//设置socket 选项
			//1.允许优雅关闭socket
			linger sLinger;
			sLinger.l_onoff = 1;	//(在closesocket()调用,但是还有数据没发送完毕的时候容许逗留)
			// 如果m_sLinger.l_onoff = 0;则功能和2.)作用相同;
			sLinger.l_linger = 2;//(容许逗留的时间为5秒)
			if (setsockopt(s, SOL_SOCKET, SO_LINGER, (const char*)&sLinger, sizeof(linger)) == SOCKET_ERROR)
			{
				bConnected = false;
				__leave;
			}

			//2.设置收发超时时限
			//int nNetTimeout = 500;//500毫秒
			//发送时限
			if (setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *)&nNetTimeout, sizeof(int)) == SOCKET_ERROR)
			{
				bConnected = false;
				__leave;
			}

			//接收时限
			if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&nNetTimeout, sizeof(int)) == SOCKET_ERROR)
			{
				bConnected = false;
				__leave;
			}

			//3.设置数据收发的缓冲区
			//在send()的时候,返回的是实际发送出去的字节(同步)或发送到socket缓冲区的字节
			//(异步);系统默认的状态发送和接收一次为8688字节(约为8.5K)；在实际的过程中发送数据
			//和接收数据量比较大,可以设置socket缓冲区,而避免了send(),recv()不断的循环收发：
			//接收缓冲区
			int nRecvBuf = 1024;//设置为1024 byte
			if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int)) == SOCKET_ERROR)
			{
				bConnected = false;
				__leave;
			}

			// 设置为非阻塞模式
			unsigned long nBlockMode = 1;
			if (ioctlsocket(s, FIONBIO, (unsigned long*)&nBlockMode) == SOCKET_ERROR)
			{
				bConnected = false;
				__leave;
			}

			sockaddr_in ServerAddr;
			ServerAddr.sin_family = AF_INET;
			ServerAddr.sin_port = htons(nPort);
			//ServerAddr.sin_addr.S_un.S_addr = inet_addr(szServerIP);  
			InetPton(AF_INET, szServerIP, &ServerAddr.sin_addr);
			if (connect(s, (sockaddr *)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR &&
				GetLastError() != WSAEWOULDBLOCK)
			{  //连接失败   
				bConnected = false;
				__leave;
			}
			struct timeval timeout;
			fd_set FDConnect;

			FD_ZERO(&FDConnect);
			FD_SET(s, &FDConnect);
			timeout.tv_sec = nNetTimeout/1000;
			timeout.tv_usec = (nNetTimeout%1000)* 1000;	//连接超时200 ms,tv_usec的时间单位是微秒
			DWORD dwTickCount = GetTickCount();
			int nRes = select(0, 0, &FDConnect, 0, &timeout);
			if (!nRes ||
				SOCKET_ERROR == nRes)
			{
				DWORD dwTimpeSpan = GetTickCount() - dwTickCount;
				bConnected = false;
				__leave;
			}
			// 重新设置为阻塞模式
			nBlockMode = 0;
			if (ioctlsocket(s, FIONBIO, (unsigned long*)&nBlockMode) == SOCKET_ERROR)
			{
				bConnected = false;
				__leave;
			}
			tLastAction = GetExactTime();
		}
		__finally
		{
			dwError = WSAGetLastError();
		}
		if (bConnected)
			return s;
		else
		{
			closesocket(s);
			SetLastError(dwError);
			return INVALID_SOCKET;
		}

	}
	int AsyncConnect(const CHAR *szServer,int nPort)
	{	
		DWORD dwErrorCode = 0;
		int nResult = SOCKET_ERROR;
		__try
		{			
			if (s != INVALID_SOCKET)
				__leave;

			if (!CreateSocket(s))
				__leave;

			int nNonBlock = 1;
			dwErrorCode		= ioctlsocket(s, FIONBIO, (unsigned long*)&nNonBlock);
			if(dwErrorCode != 0)
				__leave;

			hSocketEvent = WSACreateEvent();
			if (!hSocketEvent)
			{
				dwErrorCode = GetLastError();
				__leave;
			}
			if (SOCKET_ERROR == WSAEventSelect(s, hSocketEvent, FD_READ|FD_WRITE|FD_CONNECT|FD_CLOSE))
				__leave;

			ServerAddr.sin_port=htons((u_short)nPort);
			ServerAddr.sin_family=AF_INET;	
			ServerAddr.sin_addr.s_addr =  inet_addr((LPCSTR)szServer);

			if (::connect(s, (LPSOCKADDR)&ServerAddr, sizeof(SOCKADDR_IN)))
			{// 异步连接
				dwErrorCode = WSAGetLastError();
				if (dwErrorCode != WSAEWOULDBLOCK)
					__leave;
			}
			tLastAction = GetExactTime();
			nResult = 0;
			dwErrorCode = 0;
		}
		__finally
		{
			if (nResult != 0)
			{
				dwErrorCode = WSAGetLastError();
				if (dwErrorCode == 0)
				{
					dwErrorCode = WSAECONNREFUSED;
				}
				if (hSocketEvent)
				{
					WSACloseEvent(hSocketEvent);
					hSocketEvent = NULL;
				}
				if (s != INVALID_SOCKET)
				{
					DisConnect(s);
				}
			}
		}
		return dwErrorCode;
	}

	int Reconnect()
	{
		DWORD dwErrorCode = 0;
		int nResult = SOCKET_ERROR;
		__try
		{	
			if (s != INVALID_SOCKET)
			{
				DisConnect(s);
				s = INVALID_SOCKET;
			}
			if (hSocketEvent)
			{
				WSACloseEvent(hSocketEvent);
				hSocketEvent = NULL;
			}		

			if (!CreateSocket(s))
				__leave;

			int nNonBlock = 1;
			dwErrorCode		= ioctlsocket(s, FIONBIO, (unsigned long*)&nNonBlock);
			if(dwErrorCode != 0)
				__leave;

			hSocketEvent = WSACreateEvent();
			if (!hSocketEvent)
			{
				dwErrorCode = GetLastError();
				__leave;
			}
			if (SOCKET_ERROR == WSAEventSelect(s, hSocketEvent, FD_READ|FD_WRITE|FD_CONNECT|FD_CLOSE))
				__leave;

			if (::connect(s, (LPSOCKADDR)&ServerAddr, sizeof(SOCKADDR_IN)))
			{// 异步连接
				dwErrorCode = WSAGetLastError();
				if (dwErrorCode != WSAEWOULDBLOCK)
					__leave;
			}
			tLastAction = GetExactTime();
			nResult = 0;
			dwErrorCode = 0;
		}
		__finally
		{
			if (nResult != 0)
			{
				dwErrorCode = WSAGetLastError();
				if (dwErrorCode == 0)
				{
					dwErrorCode = WSAECONNREFUSED;
				}
				if (hSocketEvent)
				{
					WSACloseEvent(hSocketEvent);
					hSocketEvent = NULL;
				}
				if (s != INVALID_SOCKET)
				{
					DisConnect(s);
				}
			}
		}
		return dwErrorCode;

	}

	int CreateSocket(SOCKET &sClient)
	{
		bool bSucceed = true;
		__try
		{
			sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (sClient == INVALID_SOCKET)
			{
				bSucceed = false;
				__leave;
			}

			//设置socket 选项
			//1.允许优雅关闭socket
			linger sLinger;
			sLinger.l_onoff = 1;	//(在closesocket()调用,但是还有数据没发送完毕的时候容许逗留)
			// 如果m_sLinger.l_onoff = 0;则功能和2.)作用相同;
			sLinger.l_linger = 2;//(容许逗留的时间为5秒)
			if (setsockopt(sClient,SOL_SOCKET,SO_LINGER,(const char*)&sLinger,sizeof(linger)) == SOCKET_ERROR)
			{
				bSucceed = false;
				__leave;
			}

			//2.设置收发超时时限
			int nNetTimeout = 5000;//500毫秒
			//发送时限
			if (setsockopt(sClient,SOL_SOCKET,SO_SNDTIMEO,(char *)&nNetTimeout,sizeof(int)) == SOCKET_ERROR)
			{
				bSucceed = false;
				__leave;
			}

			//接收时限
			if (setsockopt(sClient,SOL_SOCKET,SO_RCVTIMEO,(char *)&nNetTimeout,sizeof(int)) == SOCKET_ERROR)
			{
				bSucceed = false;
				__leave;
			}

			//3.设置数据收发的缓冲区
			//在send()的时候,返回的是实际发送出去的字节(同步)或发送到socket缓冲区的字节
			//(异步);系统默认的状态发送和接收一次为8688字节(约为8.5K)；在实际的过程中发送数据
			//和接收数据量比较大,可以设置socket缓冲区,而避免了send(),recv()不断的循环收发：
			//接收缓冲区
			int nRecvBuf = 8192;//设置为8192 byte
			if (setsockopt(sClient,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int)) == SOCKET_ERROR)
			{
				bSucceed = false;
				__leave;
			}
// 			if (bServerMode)
// 			{
// 				SOCKADDR_IN ServerAddr;
// 				ServerAddr.sin_port = htons(nPort);
// 				ServerAddr.sin_family = AF_INET;
// 				ServerAddr.sin_addr.s_addr =  htonl(INADDR_ANY);
// 				if (bind(s,(SOCKADDR *)&ServerAddr,sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
// 				{
// 					bSucceed = false;
// 					__leave;
// 				}
// 
// 				if(listen(s, SOMAXCONN) == SOCKET_ERROR)
// 				{
// 					bSucceed = false;
// 					__leave;
// 				}
// 			}
// 			else
			bSucceed = true;
			tLastAction = GetExactTime();

		}
		__finally
		{

		}
		if (bSucceed)
			return sClient;
		else
		{
			closesocket(sClient);
			return INVALID_SOCKET;
		}
	}
	
	BOOL DisConnect(SOCKET &s)
	{
		linger sLinger;
		sLinger.l_onoff = 1;	//(在closesocket()调用,但是还有数据没发送完毕的时候容许逗留)
		// 如果m_sLinger.l_onoff = 0;则功能和2.)作用相同;
		sLinger.l_linger = 2;//优雅关闭
		__try
		{
			if (s == INVALID_SOCKET)
				__leave;
			if (setsockopt(s,SOL_SOCKET,SO_LINGER,(const char*)&sLinger,sizeof(linger)) != SOCKET_ERROR)
			{//优雅关闭
				if (shutdown(s,SD_BOTH) != SOCKET_ERROR)
				{
					char szBuff[1023] = {0};
					recv(s,szBuff,1023,0);
					closesocket(s);
					__leave;
				}
				else
				{
					shutdown(s,SD_BOTH);
					closesocket(s);
					__leave;
				}
			}
			else
			{//强制关闭
				shutdown(s,SD_BOTH);
				closesocket(s);
				__leave;
			}
		}
		__finally
		{
			s = INVALID_SOCKET;

		}
		return TRUE;
	}

	int Recv()
	{
		if (s == INVALID_SOCKET)
			return SOCKET_ERROR;
		char szTemp[8192] = {0};
		int nRecvNum = recv(s,(char *)szTemp, 8192,0);
		tLastAction = GetExactTime();
		if (nRecvNum == 0 ||nRecvNum == SOCKET_ERROR)
		{//连接关闭
			TCHAR szErrorText[255] = {0};
 			GetLastErrorMessage(_T("调用Recv函数"),szErrorText,WSAGetLastError());
 			_tprintf(szErrorText);
			return -1;
		}
		//_tprintf(_T("Recv data:%s.\n"),(byte *)szTemp);
		MemMerge(&pRecvBuffer,nBufferLength,nRecvBufferSize,szTemp,nRecvNum);
		return nBufferLength;
	}

	int Recv(byte *szBuffer,int nBufferSize)
	{
		if (!szBuffer && nBufferSize <= 0)
			return -1;

		return recv(s,(char *)szBuffer,nBufferSize,0);
	}
	int Send(byte *szBuffer,int nLeng)
	{
		if (s == INVALID_SOCKET)
			return -1;
		tLastAction = GetExactTime();
		return send(s,(const char *)szBuffer,nLeng,0);
	}

	int HttpSend(char *pContent,int nLength)
	{/*
	 HTTP/1.1 200 OK
	 Access-Control-Allow-Origin: *
	 Content-Length: 66
	 Content-Type: application/json
	 Connection: keep-alive
	 Date: Wed, 19 Apr 2017 08:49:18 GMT
	 */
		char *szContentfmt = "HTTP/1.1 200 OK\r\n"
							"Access-Control-Allow-Origin: *\r\n"
							"Content-Length: %d\r\n"
							"Content-Type: application/json\r\n"
							"Connection: keep-alive\r\n"
							"Date: %s\r\n\r\n";
		int nHeaderSize = strlen(szContentfmt) + 128;
		char *pHttpHeader = new char[nHeaderSize];
		shared_ptr<char> HeaderPtr(pHttpHeader);
		if (!pHttpHeader)
			return -1;
		char szTime[128] = {0};
		time_t ltime;   
		struct tm gmt;
		_time64( &ltime );  
		int nError = _gmtime64_s( &gmt, &ltime );  
		if (nError)  
			return -1;
		/*
		%a
		Abbreviated weekday name
		%A
		Full weekday name
		%b
		Abbreviated month name
		%B
		Full month name
		%c
		Date and time representation appropriate for locale
		%d
		Day of month as decimal number (01 – 31)
		%H
		Hour in 24-hour format (00 – 23)
		%I
		Hour in 12-hour format (01 – 12)
		%j
		Day of year as decimal number (001 – 366)
		%m
		Month as decimal number (01 – 12)
		%M
		Minute as decimal number (00 – 59)
		%p
		Current locale's A.M./P.M. indicator for 12-hour clock
		%S
		Second as decimal number (00 – 59)
		%U
		Week of year as decimal number, with Sunday as first day of week (00 – 53)
		%w
		Weekday as decimal number (0 – 6; Sunday is 0)
		%W
		Week of year as decimal number, with Monday as first day of week (00 – 53)
		%x
		Date representation for current locale
		%X
		Time representation for current locale
		%y
		Year without century, as decimal number (00 – 99)
		%Y
		Year with century, as decimal number
		%z, %Z
		Either the time-zone name or time zone abbreviation, depending on registry settings; no characters if time zone is unknown
		%%
		Percent sign
		*/
		strftime(szTime,128,"%a, %d %b %Y %H:%M:%S GMT",&gmt);
		ZeroMemory(pHttpHeader,nHeaderSize);
		sprintf(pHttpHeader, szContentfmt, nLength, szTime);

		int nHeaderLength = strlen(pHttpHeader);;
		int nHttpBuffersize = nLength + nHeaderLength + 2;

		char *pHttpBuffer = new char[nHttpBuffersize];
		shared_ptr<char> HttpBufferPtr(pHttpBuffer);
		ZeroMemory(pHttpBuffer,nHttpBuffersize);
		int nTempSize = nHttpBuffersize;
		memcpy_s(pHttpBuffer, nTempSize, pHttpHeader, nHeaderLength);
		nTempSize -= nHeaderLength;
		memcpy_s(&pHttpBuffer[nHeaderLength], nTempSize, pContent, nLength);
		nTempSize -= nLength;
		int nHttpBufferLength = nHeaderLength + nLength ;
		//pHttpBuffer[nHttpBufferLength - 1] = 0x0a;

		//TraceMsgA("%s Respond:%s.\n",__FUNCTION__,pHttpBuffer);
		int nBytesSend = Send((byte *)pHttpBuffer,nHttpBufferLength);
		if (nBytesSend == 0 || nBytesSend == SOCKET_ERROR)
			return -1;
		return true;
	}

	int HttpRecv(byte *pBuffer,int &nBuffersize)
	{
		/*
		POST /api/v1/netkbd HTTP/1.1
		Content-Length: 282
		
		Host: 192.168.1.130:33445
		Connection: keep-alive
		
		{"key_id":12,"command":"MAP_IPC","maps":[{"ipc_id":"330106100301101","monitor_id":1,"stream_type":1},{"ipc_id":"330106100301201","monitor_id":2,"stream_type":1},{"ipc_id":"330106100302401","monitor_id":3,"stream_type":1},{"ipc_id":"330106100302201","monitor_id":4,"stream_type":1}]}
		*/
		if (!pRecvBuffer)
			return -1;


		char *pHttpHeader = strstr(pRecvBuffer,"POST");
		if (pHttpHeader)
		{
			if (pHttpHeader != pRecvBuffer)
			{
				int nBreak = 3;
			}
			int nHttpLength = strlen(pHttpHeader);
			// Recved all package data			
			char *pJsonPackage = strstr((char *)pHttpHeader,"\r\n\r\n");
			if (!pJsonPackage)
				return -1;
			pJsonPackage += 4;
			int nHeaderLen = pJsonPackage - pHttpHeader;
		
			if (nHeaderLen > nBufferLength ||
				nHeaderLen > nHttpLength)
				return -1;

			//TraceMsgA("%s RecvData:%s.\n",__FUNCTION__,pRecvBuffer);
			int nJsonLength = nHttpLength - nHeaderLen;
			int nJsonLength2 = strlen(pJsonPackage);
			char *pNextHeader = strstr(pJsonPackage,"POST");
			if (pNextHeader)
			{
				nJsonLength = pNextHeader - pJsonPackage ;
			}

			char *pContenLength = strstr(pRecvBuffer,"Content-Length");
			if (!pContenLength)
				return -1;
// 			int nJsonLengthFromPack = 0;
// 			int nOffset  = pContenLength - pRecvBuffer;
// 			if (sscanf(pContenLength,"Content-Length: %d",&nJsonLengthFromPack) < 1)
// 				return -1;

			if (nBuffersize <nJsonLength)
				return -1;
			memcpy(pBuffer,&pHttpHeader[nHeaderLen],nJsonLength);
			nBuffersize = nJsonLength;
			int nProcessLen = nHeaderLen + nJsonLength;
			if (nHttpLength == nProcessLen)
			{
				ZeroMemory(pRecvBuffer,nBufferLength);
				nBufferLength =0;
			}
			else
			{
				int nRemainedLength = nHttpLength - nProcessLen;
				memmove(pRecvBuffer,&pHttpHeader[nProcessLen],nRemainedLength);
				ZeroMemory(&pRecvBuffer[nRemainedLength],nProcessLen + (pHttpHeader - pRecvBuffer));
				nBufferLength = nRemainedLength;
			}
			return 0;
		}
		else  
		{
			//////////////////////////////////////////////////////
			char *pHttpHeader = strstr(pRecvBuffer, "GET");
			if (pHttpHeader)
			{
				if (pHttpHeader != pRecvBuffer)
				{
					int nBreak = 3;
				}
				int nHttpLength = strlen(pHttpHeader);
				// Recved all package data			
				char *pJsonPackage = strstr((char *)pHttpHeader, "\r\n\r\n");
				if (!pJsonPackage)
					return -1;
				pJsonPackage += 4;
				int nHeaderLen = pJsonPackage - pHttpHeader;

				if (nHeaderLen > nBufferLength ||
					nHeaderLen > nHttpLength)
					return -1;
				int nJsonLength = nHttpLength - nHeaderLen;
				int nJsonLength2 = strlen(pJsonPackage);
				char *pNextHeader = strstr(pJsonPackage, "GET");
				if (pNextHeader)
				{
					nJsonLength = pNextHeader - pJsonPackage;
				}

				char *pContenLength = strstr(pRecvBuffer, "Content-Length");
				if (!pContenLength)
					return -1;
				// 			int nJsonLengthFromPack = 0;
				// 			int nOffset  = pContenLength - pRecvBuffer;
				// 			if (sscanf(pContenLength,"Content-Length: %d",&nJsonLengthFromPack) < 1)
				// 				return -1;

				if (nBuffersize < nJsonLength)
					return -1;
				memcpy(pBuffer, &pHttpHeader[nHeaderLen], nJsonLength);
				nBuffersize = nJsonLength;
				int nProcessLen = nHeaderLen + nJsonLength;
				if (nHttpLength == nProcessLen)
				{
					ZeroMemory(pRecvBuffer, nBufferLength);
					nBufferLength = 0;
				}
				else
				{
					int nRemainedLength = nHttpLength - nProcessLen;
					memmove(pRecvBuffer, &pHttpHeader[nProcessLen], nRemainedLength);
					ZeroMemory(&pRecvBuffer[nRemainedLength], nProcessLen + (pHttpHeader - pRecvBuffer));
					nBufferLength = nRemainedLength;
				}
				
				return 0;

			}
			else
			{
				nBuffersize = 0;
				return -1;
			}

			/////////////////////////////////////////////////////
			
			nBuffersize = 0;
			return -1;
		}
		
	}
};


typedef shared_ptr<CSockClient>CSockClientPtr;

struct ThreadInfo
{
	CRITICAL_SECTION	csThread;
	HANDLE				hThread;
	volatile bool		bRun;	
	WSAEVENT			hEventArray[64];
	CSockClient			*pClientArray[64];
	DWORD				dwEventCount;
	bool				bResumed;
	ThreadInfo()
	{
		ZeroMemory(this,sizeof(ThreadInfo));		
		InitializeCriticalSection(&csThread);		
	}
	~ThreadInfo()
	{
		TraceMsgA("%s .\n", __FUNCTION__);
		DeleteCriticalSection(&csThread);
		CloseHandle(hThread);
	}
};
typedef ThreadInfo* LPThreadInfo;
class CSocketServer
{
private:
	ThreadParam* AllocThreadParam()
	{
		return new ThreadParam;
	}
	void FreeThreadParam(ThreadParam *p)
	{
		delete p;
	}
	ThreadInfo *AllocThreadInfo()
	{
		return new ThreadInfo;
	}
	void FreeThreadInfo(ThreadInfo *p)
	{
		delete p;
	}
public:
	CRunlog*			m_pRunlog;
	LPThreadInfo		m_pThreadPool[__MaxThread];
	HANDLE				m_hListenThread;
	volatile bool		m_bListenThreadRun;
	SOCKET				m_sServer;
public:
	CSocketServer()
	{
		for (int i = 0; i < __MaxThread; i++)
			m_pThreadPool[i] = AllocThreadInfo();
		m_pRunlog = new CRunlog(_T("CSocketServer"));
		m_pRunlog->Runlog(_T("%s.\n"),__FUNCTIONW__);
		m_hListenThread = NULL;
		m_bListenThreadRun = false;
		m_sServer = INVALID_SOCKET;
	}
	virtual ~CSocketServer()
	{
		for (int i = 0; i < __MaxThread; i++)
			delete m_pThreadPool[i];
		m_pRunlog->Runlog(_T("%s.\n"),__FUNCTIONW__);
		delete m_pRunlog;
	}
	static bool InitializeWinsock()
	{
		WSADATA		WSAData = { 0 };
		if ( WSAStartup( WSA_VERSION, &WSAData ) )
		{
			if ( LOBYTE( WSAData.wVersion ) != LOBYTE(WSA_VERSION) ||
				HIBYTE( WSAData.wVersion ) != HIBYTE(WSA_VERSION) )
			{
				WSACleanup();
				return false;
			}
		}
		return true;
	}

	bool Start(USHORT nPort = 80)
	{
		bool bSucceed = false;
		ThreadParam * ThreadArray[__MaxThread] = {0};
		__try
		{
			if(CreateSocket(m_sServer,nPort) == INVALID_SOCKET)
			{
				__leave;
			}

			m_bListenThreadRun = true;

			m_hListenThread = (HANDLE)_beginthreadex(NULL,0,ListenThread,this,0,0);
			if (!m_hListenThread)
				__leave;
			
			DWORD dwThreadID = 0;
			for (int i = 0;i < __MaxThread;i ++)
			{
				ThreadParam *pParam = AllocThreadParam();
				ThreadArray[i] = pParam;
				pParam->pParam = this;
				pParam->nThreadIndex = i;
				m_pThreadPool[i]->bRun = true;
				m_pThreadPool[i]->dwEventCount = 0;
				m_pThreadPool[i]->hThread = (HANDLE)_beginthreadex(NULL,0,MonitorThread,pParam,CREATE_SUSPENDED,0);		
				if (!m_pThreadPool[i]->hThread)
				{
					delete pParam;
					__leave;
				}
			}
			bSucceed = true;
		}
		__finally
		{
			if (!bSucceed)
			{
				m_bListenThreadRun = false;
				HANDLE hWaitArray[64] = {0};
				int nWaitCount = 0;
				if (m_hListenThread)
				{
					hWaitArray[nWaitCount ++] = m_hListenThread;
				}
				for (int i = 0;i < __MaxThread;i ++)
				{
					m_pThreadPool[i]->bRun = false;
					if (m_pThreadPool[i]->hThread)
						hWaitArray[nWaitCount ++] = m_pThreadPool[i]->hThread;
				}
				WaitForMultipleObjects(nWaitCount,hWaitArray,TRUE,INFINITE);
				for (int i = 0;i < __MaxThread;i ++)
					FreeThreadParam(ThreadArray[i]);
			}
		}
		return bSucceed;
	}
	virtual void Stop()
	{
		if (!m_hListenThread)
			return  ;
		m_bListenThreadRun = false;
		DisConnect(m_sServer);
		
		HANDLE hWaitArray[64] = {0};
		int nWaitCount = 0;
		
		hWaitArray[nWaitCount ++] = m_hListenThread;
		for (int i = 0;i < __MaxThread;i ++)
		{
			LPThreadInfo pThread = m_pThreadPool[i];
			if (pThread->hThread)
			{	
				pThread->bRun = false;		
				for (DWORD n = 0;n <pThread->dwEventCount; n++)
				{
					DisConnect(pThread->pClientArray[n]->s);
				}
				
				hWaitArray[nWaitCount ++] = pThread->hThread;
				// 唤醒所有线程
				ResumeThread(pThread->hThread);	
				SetThreadPriority(pThread->hThread,THREAD_PRIORITY_ABOVE_NORMAL);
			}
		}
		
		WaitForMultipleObjects(nWaitCount,hWaitArray,TRUE,INFINITE); 
		CloseHandle(m_hListenThread);		
		m_hListenThread = NULL;
		m_sServer = INVALID_SOCKET;
		
	}

	
	static int CreateSocket(SOCKET &s,WORD nPort)
	{
		bool bListen = true;
		__try
		{
			s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (s == INVALID_SOCKET)
			{
				bListen = false;
				__leave;
			}

			//设置socket 选项
			//1.允许优雅关闭socket
			linger sLinger;
			sLinger.l_onoff = 1;	//(在closesocket()调用,但是还有数据没发送完毕的时候容许逗留)
			// 如果m_sLinger.l_onoff = 0;则功能和2.)作用相同;
			sLinger.l_linger = 2;//(容许逗留的时间为5秒)
			if (setsockopt(s,SOL_SOCKET,SO_LINGER,(const char*)&sLinger,sizeof(linger)) == SOCKET_ERROR)
			{
				bListen = false;
				__leave;
			}

			//2.设置收发超时时限
			int nNetTimeout = 5000;//500毫秒
			//发送时限
			if (setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,(char *)&nNetTimeout,sizeof(int)) == SOCKET_ERROR)
			{
				bListen = false;
				__leave;
			}

			//接收时限
			if (setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(char *)&nNetTimeout,sizeof(int)) == SOCKET_ERROR)
			{
				bListen = false;
				__leave;
			}

			//3.设置数据收发的缓冲区
			//在send()的时候,返回的是实际发送出去的字节(同步)或发送到socket缓冲区的字节
			//(异步);系统默认的状态发送和接收一次为8688字节(约为8.5K)；在实际的过程中发送数据
			//和接收数据量比较大,可以设置socket缓冲区,而避免了send(),recv()不断的循环收发：
			//接收缓冲区
			int nRecvBuf = 8192;//设置为8192 byte
			if (setsockopt(s,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int)) == SOCKET_ERROR)
			{
				bListen = false;
				__leave;
			}
			int nOption = 1;
			if( setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&nOption, sizeof(nOption)) == SOCKET_ERROR)  
			{  
				bListen = false;
				__leave;
			}  

			SOCKADDR_IN ServerAddr;
			ServerAddr.sin_port = htons(nPort);
			ServerAddr.sin_family = AF_INET;
			ServerAddr.sin_addr.s_addr =  htonl(INADDR_ANY);
			if (::bind(s,(SOCKADDR *)&ServerAddr,sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
			{
				bListen = false;
				__leave;
			}

			if(listen(s, SOMAXCONN) == SOCKET_ERROR)
			{
				bListen = false;
				__leave;
			}

		}
		__finally
		{

		}
		if (bListen)
			return s;
		else
		{
			closesocket(s);
			return INVALID_SOCKET;
		}
	}

	static BOOL DisConnect(SOCKET &s)
	{
		linger sLinger;
		sLinger.l_onoff = 1;	//(在closesocket()调用,但是还有数据没发送完毕的时候容许逗留)
		// 如果m_sLinger.l_onoff = 0;则功能和2.)作用相同;
		sLinger.l_linger = 2;//优雅关闭
		__try
		{
			if (s == INVALID_SOCKET)
				__leave;
			if (setsockopt(s,SOL_SOCKET,SO_LINGER,(const char*)&sLinger,sizeof(linger)) != SOCKET_ERROR)
			{//优雅关闭
				if (shutdown(s,SD_BOTH) != SOCKET_ERROR)
				{
					char szBuff[1023] = {0};
					recv(s,szBuff,1023,0);
					closesocket(s);
					__leave;
				}
				else
				{
					shutdown(s,SD_BOTH);
					closesocket(s);
					__leave;
				}
			}
			else
			{//强制关闭
				shutdown(s,SD_BOTH);
				closesocket(s);
				__leave;
			}
		}
		__finally
		{
			s = INVALID_SOCKET;

		}
		return TRUE;
	}


	static UINT __stdcall ListenThread(void *p)
	{
		CSocketServer *pThis = (CSocketServer *)p;
		SOCKET hNewClient = INVALID_SOCKET;
		sockaddr_in ClientAddr;
		TCHAR szText[255] = {0};
		CRunlog *pRunlog = pThis->m_pRunlog;
		int nErrorCode = 0;
		int nSockAddrLen = sizeof(sockaddr_in);
		while(pThis->m_bListenThreadRun)
		{
			hNewClient = accept(pThis->m_sServer,(sockaddr *)&ClientAddr,&nSockAddrLen);
			if(hNewClient == INVALID_SOCKET)
			{
				nErrorCode = WSAGetLastError();
				//GetLastErrorMessageA(_T("侦听线程:Call Accept"),szText,nErrorCode);
				//PushMsg(szText);
				Sleep(5);
				continue;
			}
			else
			{
				TraceMsg(_T("Client:%d.%d.%d.%d:%d Connected.\n"),
					ClientAddr.sin_addr.S_un.S_un_b.s_b1,
					ClientAddr.sin_addr.S_un.S_un_b.s_b2,
					ClientAddr.sin_addr.S_un.S_un_b.s_b3,
					ClientAddr.sin_addr.S_un.S_un_b.s_b4,
					ClientAddr.sin_port);
				if (pThis->m_pRunlog)
					pThis->m_pRunlog->Runlog(_T("Client:%d.%d.%d.%d:%d Connected.\n"),
					ClientAddr.sin_addr.S_un.S_un_b.s_b1,
					ClientAddr.sin_addr.S_un.S_un_b.s_b2,
					ClientAddr.sin_addr.S_un.S_un_b.s_b3,
					ClientAddr.sin_addr.S_un.S_un_b.s_b4,
					ClientAddr.sin_port);

				CSockClient *pClient = new CSockClient(hNewClient,ClientAddr);	
				pClient->pRunlog = pThis->m_pRunlog;
				bool bAllocated =  false;
				bool bResumeThread =  false;
				pThis->OnAccept(pClient);

				for (int i = 0; i < __MaxThread;i ++)
				{
					LPThreadInfo pThreadInfo = pThis->m_pThreadPool[i];
					EnterCriticalSection(&pThreadInfo->csThread);
					if (pThreadInfo->dwEventCount < __MaxClients)
					{
						pThreadInfo->pClientArray[pThreadInfo->dwEventCount] = pClient;					
						pThreadInfo->hEventArray[pThreadInfo->dwEventCount] = pClient->hSocketEvent;
						pThreadInfo->dwEventCount ++;
						if (pThreadInfo->dwEventCount == 1 &&	// 新添加的第一个事件,则唤醒该线程
							!pThreadInfo->bResumed)			
						{
							bResumeThread = true;
						}
						bAllocated = true;
					}
					LeaveCriticalSection(&pThreadInfo->csThread);
					if (bResumeThread && p)
					{
						TraceMsg(_T("MonitorThread[%d] is Resumed.\r\n"),i);
						ResumeThread(pThreadInfo->hThread);
					}
					if (bAllocated )
						break;				
				}
				

			}
		}
		return 0;
	}
	
	virtual int OnAccept(CSockClient *pClient) = 0;
	virtual int OnRecv(CSockClient *pClient) = 0;
	virtual int OnWrite(CSockClient *pClient) = 0;
	virtual int OnDisConnect(CSockClient *pClient) = 0;
	//  Recv data and process data
	int ProcessEvent(LPThreadInfo pThreadInfo,DWORD dwIndex)
	{
		WSAEVENT hClientEvent = pThreadInfo->hEventArray[dwIndex];
		CSockClient *pClient = pThreadInfo->pClientArray[dwIndex];
		
		WSANETWORKEVENTS NetworkEvents;
		WSAEnumNetworkEvents(pClient->s, hClientEvent, &NetworkEvents);
		WSAResetEvent(hClientEvent);

		byte dzDataBlock[1024] = {0};
		char szHexText[4096] = {0};
		int nDabaBlockLength = 0;
		int nNecessarySize = 220;		// data block size
		pClient->tLastAction = GetExactTime();		// update the SOCKET heartbeat 
		if (NetworkEvents.lNetworkEvents & FD_READ)
		{
			return OnRecv(pClient);
			
		}
		if (NetworkEvents.lNetworkEvents & FD_WRITE)
		{// send succeed
			// to do something for sending
			return OnWrite(pClient);
		}
		if (NetworkEvents.lNetworkEvents & FD_CONNECT)
		{// connect succeed
			TraceMsgA("Clinet[%d] Connected Succeed.\n",dwIndex);
		}
		if (NetworkEvents.lNetworkEvents & FD_CLOSE)
		{// socket is closed
			return RemoveConnect(pThreadInfo,dwIndex);
		}
		return 0;
	}

	void CheckFreeClient(LPThreadInfo pThreadInfo,double dfTimeout = 0.100f)
	{
		::EnterCriticalSection(&pThreadInfo->csThread);
		for (int nIndex = pThreadInfo->dwEventCount - 1; nIndex >= 0;nIndex --)
		{
			CSockClient *pClient = pThreadInfo->pClientArray[nIndex];
			//TraceMsgA("%s TimeSpan = %.3f.\n", __FUNCTION__, TimeSpanEx(pClient->tLastAction));
			if (TimeSpanEx(pClient->tLastAction) >= dfTimeout)
				RemoveConnect(pThreadInfo, nIndex);
		}
		::LeaveCriticalSection(&pThreadInfo->csThread);
	}
	static UINT __stdcall MonitorThread(void *p)
	{
		ThreadParam *pThreadParam = (ThreadParam *)p;
		int &nThreadIndex = pThreadParam->nThreadIndex;
		CSocketServer *pThis =  (CSocketServer *)pThreadParam->pParam;
		LPThreadInfo pThreadInfo = pThis->m_pThreadPool[nThreadIndex];
		CRunlog *pRunlog = pThis->m_pRunlog;
		volatile DWORD &dwEventCount = pThreadInfo->dwEventCount;
		SOCKET sClient = INVALID_SOCKET;
		WSAEVENT hClientEvent = NULL;
		DWORD dwIndex = 0;	
		CSockClient *pClient = NULL;
		TCHAR szDateTime[31] = {0};

		bool bWait = false;
		while(pThreadInfo->bRun)
		{
			EnterCriticalSection(&pThreadInfo->csThread);
			if (!pThreadInfo->dwEventCount)
				bWait = true;
			else
				bWait = false;
			LeaveCriticalSection(&pThreadInfo->csThread);
			if (bWait)
			{
				Sleep(10);
				bWait = false;
				continue;
			}

			DWORD dwRet = WSAWaitForMultipleEvents(dwEventCount, pThreadInfo->hEventArray, FALSE, 50, FALSE);
			if (dwRet == WSA_WAIT_FAILED)
			{
				DWORD dwError = WSAGetLastError();
				switch(dwError)
				{
				case WSANOTINITIALISED:
					TraceMsgA("A successful WSAStartup call must occur before using this function.\n");
					return 0;
				case WSAENETDOWN:
					TraceMsgA("network subsystem has failed.\n");
					return 0;
				case WSAEINPROGRESS:
					TraceMsgA("A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.\n");
					break;
				case WSA_NOT_ENOUGH_MEMORY:
					TraceMsgA("enough free memory was available to complete the operation.\n");
					break;
				case WSA_INVALID_HANDLE:
					TraceMsgA("One or more of the values in the lphEvents array is not a valid event object handle.\n");
					break;
				case WSA_INVALID_PARAMETER:
					{
						TraceMsgA("cEvents parameter does not contain a valid handle count.\n");
						Sleep(10);
					}
					break;
				}
				continue;
			}
			if (dwRet == WSA_WAIT_TIMEOUT)
			{
				// 检查并删除闲置连接
				pThis->CheckFreeClient(pThreadInfo,0.200f);
				continue;
			}
			dwIndex = dwRet - WSA_WAIT_EVENT_0;

			if (pThis->ProcessEvent(pThreadInfo,dwIndex) == -1)
			{//移除该客户连接
				TraceMsg(_T("MonitorThread[%d]的Client[%d]Disconnected.\r\n"),nThreadIndex,dwIndex);
				pRunlog->Runlog(_T("MonitorThread[%d]的Client[%d]Disconnected.\r\n"),nThreadIndex,dwIndex);
				pThis->RemoveConnect(pThreadInfo,dwIndex);			
			}
			else
				dwIndex ++;

			while (dwIndex < dwEventCount)
			{
				dwRet = WSAWaitForMultipleEvents(dwEventCount - dwIndex, &pThreadInfo->hEventArray[dwIndex], FALSE, 0, FALSE);
				if (dwRet == WSA_WAIT_TIMEOUT)
				{
					dwIndex = dwEventCount;
					break;
				}
				dwIndex += (dwRet - WSA_WAIT_EVENT_0);
				if (pThis->ProcessEvent(pThreadInfo,dwIndex) == -1)
				{//移除该客户连接
					TraceMsg(_T("MonitorThread[%d]的Client[%d]Disconnected.\r\n"),nThreadIndex,dwIndex);
					pRunlog->Runlog(_T("MonitorThread[%d]的Client[%d]Disconnected.\r\n"),nThreadIndex,dwIndex);
					pThis->RemoveConnect(pThreadInfo,dwIndex);			
				}
				else
					dwIndex ++;
			}
		}
		delete pThreadParam;
		return 0L;
	}

	int RemoveConnect(LPThreadInfo pThreadInfo,DWORD dwIndex)
	{		
		WSAEVENT hClientEvent = pThreadInfo->hEventArray[dwIndex];
		CSockClient *pClient = pThreadInfo->pClientArray[dwIndex];
		::EnterCriticalSection(&pThreadInfo->csThread);
		pThreadInfo->dwEventCount --;
		memmove(&pThreadInfo->hEventArray[dwIndex],&pThreadInfo->hEventArray[dwIndex + 1],(pThreadInfo->dwEventCount - dwIndex)*sizeof(WSAEVENT));
		pThreadInfo->hEventArray[pThreadInfo->dwEventCount] = NULL;
		memmove(&pThreadInfo->pClientArray[dwIndex],&pThreadInfo->pClientArray[dwIndex + 1],(pThreadInfo->dwEventCount - dwIndex)*sizeof(CSockClient*));
		pThreadInfo->pClientArray[pThreadInfo->dwEventCount] = NULL;
		::LeaveCriticalSection(&pThreadInfo->csThread);

		TraceMsg(_T("%s Client:%d.%d.%d.%d:%d Disconnected(%d).\r\n"),
			__FUNCTIONW__,
			pClient->ClientAddr.sin_addr.S_un.S_un_b.s_b1,
			pClient->ClientAddr.sin_addr.S_un.S_un_b.s_b2,
			pClient->ClientAddr.sin_addr.S_un.S_un_b.s_b3,
			pClient->ClientAddr.sin_addr.S_un.S_un_b.s_b4,
			pClient->ClientAddr.sin_port,
			pThreadInfo->dwEventCount);
		pClient->pRunlog->Runlog(_T("%s Client:%d.%d.%d.%d:%d Disconnected.\r\n"),
			__FUNCTIONW__,
			pClient->ClientAddr.sin_addr.S_un.S_un_b.s_b1,
			pClient->ClientAddr.sin_addr.S_un.S_un_b.s_b2,
			pClient->ClientAddr.sin_addr.S_un.S_un_b.s_b3,
			pClient->ClientAddr.sin_addr.S_un.S_un_b.s_b4,
			pClient->ClientAddr.sin_port);
		DisConnect(pClient->s);
		OnDisConnect(pClient);
		delete pClient;
		return 0;
	}
};



#endif	//QSOCKETSERVER_H
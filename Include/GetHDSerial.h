// GetHDSerial.h: interface for the CGetHDSerial class. 
// 
////////////////////////////////////////////////////////////////////// 

#pragma once
#include <windows.h>
#include <WinIoCtl.h>
#include <iptypes.h>
#include <IPHlpApi.h>
#include <stdio.h>
#include "atlbase.h"
#include "atlstr.h"
#define  SENDIDLENGTH  sizeof (SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE 
#define  IDENTIFY_BUFFER_SIZE  512 
#define  FILE_DEVICE_SCSI              0x0000001b 
#define  IOCTL_SCSI_MINIPORT_IDENTIFY  ((FILE_DEVICE_SCSI << 16) + 0x0501) 
#define  IOCTL_SCSI_MINIPORT 0x0004D008  
#define  IDE_ATAPI_IDENTIFY  0xA1   
#define  IDE_ATA_IDENTIFY    0xEC  
#define  IOCTL_GET_DRIVE_INFO   0x0007c088 
#define  IOCTL_GET_VERSION          0x00074080 
typedef struct _IDSECTOR 
{ 
   USHORT  wGenConfig; 
   USHORT  wNumCyls; 
   USHORT  wReserved; 
   USHORT  wNumHeads; 
   USHORT  wBytesPerTrack; 
   USHORT  wBytesPerSector; 
   USHORT  wSectorsPerTrack; 
   USHORT  wVendorUnique[3]; 
   CHAR    sSerialNumber[20]; 
   USHORT  wBufferType; 
   USHORT  wBufferSize; 
   USHORT  wECCSize; 
   CHAR    sFirmwareRev[8]; 
   CHAR    sModelNumber[40]; 
   USHORT  wMoreVendorUnique; 
   USHORT  wDoubleWordIO; 
   USHORT  wCapabilities; 
   USHORT  wReserved1; 
   USHORT  wPIOTiming; 
   USHORT  wDMATiming; 
   USHORT  wBS; 
   USHORT  wNumCurrentCyls; 
   USHORT  wNumCurrentHeads; 
   USHORT  wNumCurrentSectorsPerTrack; 
   ULONG   ulCurrentSectorCapacity; 
   USHORT  wMultSectorStuff; 
   ULONG   ulTotalAddressableSectors; 
   USHORT  wSingleWordDMA; 
   USHORT  wMultiWordDMA; 
   BYTE    bReserved[128]; 
} IDSECTOR, *PIDSECTOR; 
 
typedef struct _SRB_IO_CONTROL 
{ 
   ULONG HeaderLength; 
   UCHAR Signature[8]; 
   ULONG Timeout; 
   ULONG ControlCode; 
   ULONG ReturnCode; 
   ULONG Length; 
} SRB_IO_CONTROL, *PSRB_IO_CONTROL; 
 
typedef struct _GETVERSIONOUTPARAMS 
{ 
   BYTE bVersion;     
   BYTE bRevision;    
   BYTE bReserved;    
   BYTE bIDEDeviceMap; 
   DWORD fCapabilities; 
   DWORD dwReserved[4]; 
} GETVERSIONOUTPARAMS, *PGETVERSIONOUTPARAMS, *LPGETVERSIONOUTPARAMS; 

// 获取硬盘序列号的类 
class CHardwareID   
{ 
public: 
	CHardwareID(); 
	virtual ~CHardwareID(); 
	void  _stdcall Win9xReadHDSerial(WORD * buffer); 
	bool GetHDSerial(char *szHDSerial,int nSize); 
	char* WORDToChar (WORD diskdata [256], int firstIndex, int lastIndex); 
	char* DWORDToChar (DWORD diskdata [256], int firstIndex, int lastIndex); 
	BOOL  WinNTReadSCSIHDSerial(DWORD * buffer); 
	BOOL  WinNTReadIDEHDSerial (DWORD * buffer); 
	BOOL  WinNTGetIDEHDInfo (HANDLE hPhysicalDriveIOCTL, PSENDCMDINPARAMS pSCIP, 
                      PSENDCMDOUTPARAMS pSCOP, BYTE bIDCmd, BYTE bDriveNum, 
                      PDWORD lpcbBytesReturned); 
	// 是否实体网卡
	bool IsRealNetAdapter ( const char *pAdapterName )
	{
		BOOL ret_value = FALSE;

#define NET_CARD_KEY "System\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}"
		char szDataBuf[1024];
		DWORD dwDataLen = 1024;
		DWORD dwType = REG_SZ;
		HKEY hNetKey = NULL;
		HKEY hLocalNet = NULL;
		DWORD dwErrorCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT(NET_CARD_KEY), 0, KEY_ALL_ACCESS, &hNetKey);
		if(ERROR_SUCCESS != dwErrorCode)
		{
			DWORD dwError = GetLastError();
			return FALSE;
		}
		sprintf(szDataBuf, "%s\\Connection", pAdapterName);
		//USES_CONVERSION;  
		//LPCWSTR wszDataBuf = CA2W(szDataBuf);  
		LPCWSTR wszDataBuf = CA2T(szDataBuf);
		if(ERROR_SUCCESS != RegOpenKeyEx(hNetKey ,wszDataBuf,0 ,KEY_READ, &hLocalNet))
		{
			RegCloseKey(hNetKey);
			return FALSE;
		}
		//  win10  系统没有MediaSubType项
		// 	if (ERROR_SUCCESS != RegQueryValueEx(hLocalNet, "MediaSubType", 0, &dwType, (BYTE *)szDataBuf, &dwDataLen))
		// 	{
		// 		goto ret;
		// 	}
		// 	if (*((DWORD *)szDataBuf)!=0x01)
		// 		goto ret;
		dwDataLen = 1024;
		char* dataBuffV = "PnpInstanceID";
		LPCWSTR wszDataBufV = CA2T(dataBuffV);
		TCHAR szData[MAX_PATH];
		if (ERROR_SUCCESS != RegQueryValueEx(hLocalNet, wszDataBufV, 0, &dwType, (LPBYTE)szData, &dwDataLen))
		{
			goto ret;
		}

		if(_tcsncmp(szData,TEXT("PCI"),strlen("PCI")))
			goto ret;

		ret_value = TRUE;

ret:
		RegCloseKey(hLocalNet);
		RegCloseKey(hNetKey);

		return ret_value!=0;

	}

	int GetMacAddress(char szMac[][16],int &nCount)  
	{
#ifdef LINUX
		int num = 0;
		int sock = INVALID_SOCKET;
		do 
		{
			sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
			if (sock == -1)
			{
				break;
			}

			char buf[1024] = {0};
			struct ifconf ifc;
			ifc.ifc_len = sizeof(buf);
			ifc.ifc_buf = buf;
			if (ioctl(sock, SIOCGIFCONF, &ifc) == -1)
			{
				break;
			}

			struct ifreq ifr;
			struct ifreq* it = ifc.ifc_req;
			const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));
			for (; it != end; ++it)
			{
				strcpy(ifr.ifr_name, it->ifr_name);
				if (ioctl(sock, SIOCGIFFLAGS, &ifr) == -1)
				{
					continue;
				}

				if (ifr.ifr_flags & IFF_LOOPBACK) // don't count loopback
				{
					continue;
				}

				if (ioctl(sock, SIOCGIFHWADDR, &ifr) == -1)
				{
					continue;
				}

				sprintf(szMac[num++], "%02X%02X%02X%02X%02X%02X", 
					(unsigned char)ifr.ifr_hwaddr.sa_data[0], 
					(unsigned char)ifr.ifr_hwaddr.sa_data[1], 
					(unsigned char)ifr.ifr_hwaddr.sa_data[2], 
					(unsigned char)ifr.ifr_hwaddr.sa_data[3], 
					(unsigned char)ifr.ifr_hwaddr.sa_data[4], 
					(unsigned char)ifr.ifr_hwaddr.sa_data[5]);
			}
		} while (0);

		if (INVALID_SOCKET != sock)
		{
			closesocket(sock);
			sock = INVALID_SOCKET;
		}

		return num;
#else
		PIP_ADAPTER_INFO pAdapterInfo;  
		DWORD AdapterInfoSize;   
		DWORD Err;    
		AdapterInfoSize   =   0;  
		Err = GetAdaptersInfo(NULL,   &AdapterInfoSize);  
		if( ( Err != 0 ) && ( Err != ERROR_BUFFER_OVERFLOW ) )
		{  
			return -1;
		}  
		//分配网卡信息内存  
		pAdapterInfo = (PIP_ADAPTER_INFO)GlobalAlloc(GPTR, AdapterInfoSize);  
		if( pAdapterInfo == NULL )
		{  
			return -1;  
		}    
		if( GetAdaptersInfo( pAdapterInfo, &AdapterInfoSize ) != 0 )
		{  
			GlobalFree(pAdapterInfo);  
			return -1;  
		}

		int nIndex = 0;
		while ( pAdapterInfo )
		{
			if (IsRealNetAdapter(pAdapterInfo->AdapterName))
			{
// 				sprintf(szMac[nIndex ++], "%02X%02X%02X%02X%02X%02X", 
// 					pAdapterInfo->Address[0],  
// 					pAdapterInfo->Address[1],  
// 					pAdapterInfo->Address[2],  
// 					pAdapterInfo->Address[3],  
// 					pAdapterInfo->Address[4],  
// 					pAdapterInfo->Address[5]);
				memcpy(szMac[nIndex ++],pAdapterInfo->Address,6);
			}
			pAdapterInfo = pAdapterInfo->Next;
		}

		GlobalFree(pAdapterInfo);  
		return nIndex; 
#endif
	}

	BOOL GetCPUID(UINT64* szCPUID,CHAR* szVenderID,int nVenderSize);
}; 

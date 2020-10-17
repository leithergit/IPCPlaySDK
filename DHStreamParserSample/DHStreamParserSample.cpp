// DHStreamParserSample.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <memory>
#include <list>
#include "svacparser/DhStreamParser.h"

enum IPC_FRAME_TYPE{
	IPC_GOV_FRAME = -1, ///< GOVÖ¡
	IPC_IDR_FRAME = 1,  ///< IDRÖ¡¡£
	IPC_I_FRAME,        ///< IÖ¡¡£
	IPC_P_FRAME,        ///< PÖ¡¡£
	IPC_B_FRAME,        ///< BÖ¡¡£
	IPC_JPEG_FRAME,     ///< JPEGÖ¡
	IPC_711_ALAW,       ///< 711 AÂÉ±àÂëÖ¡
	IPC_711_ULAW,       ///< 711 UÂÉ±àÂëÖ¡
	IPC_726,            ///< 726±àÂëÖ¡
	IPC_AAC,            ///< AAC±àÂëÖ¡¡£
	IPC_MAX,
} ;

using namespace std;
using namespace std::tr1;
typedef shared_ptr <DH_FRAME_INFO> DH_FRAME_INFO_PTR;
list<DH_FRAME_INFO_PTR> FrameList;
shared_ptr<DhStreamParser> pDHStreamParser = nullptr;
int _tmain(int argc, _TCHAR* argv[])
{
	if (!pDHStreamParser)
	{
		pDHStreamParser = make_shared<DhStreamParser>();
	}


}


int DataCallBack(unsigned char *pBuffer, unsigned long nLength, void *pUser)
{
	pDHStreamParser->InputData(pBuffer, nLength);
	DH_FRAME_INFO *pDHFrame = nullptr;
	do
	{
		pDHFrame = pDHStreamParser->GetNextFrame();
		if (!pDHFrame)	// ÒÑ¾­Ã»ÓÐÊý¾Ý¿ÉÒÔ½âÎö£¬ÖÕÖ¹Ñ­»·
			break;
		if (pDHFrame->nType != DH_FRAME_TYPE_VIDEO)
			break;
// 		int nStatus = InputStream(pDHFrame->pContent,
// 			pDHFrame->nSubType == DH_FRAME_TYPE_VIDEO_I_FRAME ? IPC_I_FRAME : IPC_P_FRAME,
// 			pDHFrame->nLength,
// 			pDHFrame->nRequence,
// 			(time_t)pDHFrame->nTimeStamp);
// 		if (0 == nStatus)
// 		{
// 			pDHFrame = nullptr;
// 			continue;
// 		}
// 		else
// 			return nStatus;

	} while (true);
	return 0;
}
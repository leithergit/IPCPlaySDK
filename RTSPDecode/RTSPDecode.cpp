// RTSPDecode.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "VideoDecoder/VideoDecoder.h"
#include <memory>
#include <MMSystem.h>
#include <process.h>
using namespace std;
using namespace std::tr1;
#pragma comment(lib,"winmm.lib")
#define TestFile	"D:\\Git\\live555\\VCSample\\testRTSPClient\\test.mp4"
bool bThreadRun = false;
UINT __stdcall Thread(void *p)
{
	CAvRegister avregister;
	shared_ptr<CVideoDecoder> pDecoderPtr = make_shared<CVideoDecoder>();
	
	AVPacket *pAvPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
	shared_ptr<AVPacket>AvPacketPtr(pAvPacket, av_free);
	av_init_packet(pAvPacket);
	AVFrame *pAvFrame = av_frame_alloc();
	shared_ptr<AVFrame>AvFramePtr(pAvFrame, av_free);

	int nGetFrame = 0;
	DWORD dwT1 = timeGetTime();
	int nCount = 1;
	int nFrames = 0;
	int nPlayCount = 0;

	pDecoderPtr->LoadDecodingFile(TestFile, false);
	int nDecodedFrames = 0;

	while (bThreadRun)
	{
		if ((timeGetTime() - dwT1) >= 5000*nCount)
		{
			nCount++;
			printf("Eplase time %d second,Frames = %d.\n", (timeGetTime() - dwT1)/1000,nFrames);
		}
		nFrames++;
		if (pDecoderPtr->ReadFrame(pAvPacket) != 0)
		{
			// Ñ­»·²¥·Å
			nPlayCount++;
			pDecoderPtr->SeekFrame(0, AVSEEK_FLAG_BYTE);
			printf("Seek to Header,TimeSpan = %.3f\tFrames = %d\tnPlayCount = %d\n",  (timeGetTime() - dwT1) / 1000.0f,nFrames, nPlayCount);
			nFrames = 0;
			break;
		}
		
		if (pDecoderPtr->Decode(pAvFrame, nGetFrame, pAvPacket) == 0)
		{
			nDecodedFrames++;
			if (!nGetFrame)
				printf("Decode succed,but not get a frame.\n");
		}
		av_frame_unref(pAvFrame);
		av_packet_unref(pAvPacket);
		//Sleep(5);
	}
	DWORD dwTimeSpan = timeGetTime() - dwT1;
	int nFPS = nDecodedFrames * 1000 / dwTimeSpan;
	printf("%s nFPS = %d.\n",__FUNCTION__, nFPS);
	av_frame_unref(pAvFrame);
	av_packet_unref(pAvPacket);
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	
	bThreadRun = true;
	HANDLE hThread = (HANDLE)_beginthreadex(nullptr, 0, Thread, nullptr, 0, nullptr);
	
	printf("\n");
	system("pause");
	bThreadRun = false;
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	return 0;
}


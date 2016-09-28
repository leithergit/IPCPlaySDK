#if !defined(PSSTREAM_H)
#define PSSTREAM_H

#include "StreamParser.h"
#include <time.h>

#define  PSIFRAMELEN 512000
#define  AUDIOFRAMELEN 4096

#ifdef WIN32
#define int64_t __int64
#else
#define int64_t long long
#endif

class PSStream : public StreamParser  
{
public:
	PSStream(unsigned char* rawBuf);
	virtual ~PSStream();

	bool CheckSign(const unsigned int& Code) ;
	bool ParseOneFrame() ;
	bool CheckIfFrameValid() ; 
	int Reset(int level);

public:
	DH_FRAME_INFO *GetNextFrame() ;

private:
	//用于记录I帧第一个包的信息
	unsigned char m_buffer[PSIFRAMELEN];
	unsigned char* m_PSBuf;
	unsigned int m_PSRemainLen;
	DH_FRAME_INFO* m_pLastFrame;
	DH_FRAME_INFO m_tmpFrame;
    int audioType;
	int videoType;
	time_t m_beginTime;
	unsigned char m_audioBuf[AUDIOFRAMELEN];
	static unsigned int adts_sample_rates[];
	unsigned char psmBuf[96];
	int psmPos;
	int setPsm;

private:
	int makeVideoFrame(DH_FRAME_INFO* frame);
	int makeAudioFrame(DH_FRAME_INFO* frame);
};

#endif // !defined(PSSTREAM_H)

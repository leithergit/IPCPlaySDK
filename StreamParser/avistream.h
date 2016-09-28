#if !defined(AVISTREAM_H)
#define AVISTREAM_H

#include "StreamParser.h"

class AVIStream : public StreamParser  
{
public:
	AVIStream(unsigned char* rawBuf, DH_FRAME_INFO* AviInfo);
	virtual ~AVIStream();
	
	bool CheckSign(const unsigned int& Code) ;
	bool ParseOneFrame() ;
	bool CheckIfFrameValid() ; 
private:
	int m_width;
	int m_height;
	int m_rate;
	int m_bitsPerSample;
	int m_samplesPerSecond;
};

#endif // !defined(AVISTREAM_H)
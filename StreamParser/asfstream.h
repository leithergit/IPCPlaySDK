#if !defined(ASFSTREAM_H)
#define ASFSTREAM_H

#include "StreamParser.h"
#include "newstream.h"

class ASFStream : public StreamParser  
{
public:
	ASFStream(unsigned char* rawBuf, int packetlen, int videoID);
	virtual ~ASFStream();

	bool CheckSign(const unsigned int& Code) ;
	bool ParseOneFrame() ;
	bool CheckIfFrameValid() ; 

	DH_FRAME_INFO *GetNextFrame() ;
	DH_FRAME_INFO *GetNextKeyFrame() ;
private:
	int m_packetlen;
	NewStream* m_newstream;
	int m_videoID;
	unsigned char m_tmpBuf[MAX_BUFFER_SIZE+64*1024];
	unsigned int m_tmpLen;
	unsigned char m_newstreamBuf[MAX_BUFFER_SIZE];
};

#endif // !defined(ASFSTREAM_H)
#if !defined(TSSTREAM_H)
#define TSSTREAM_H

#include "StreamParser.h"

class TSStream : public StreamParser  
{
public:
	TSStream(unsigned char* rawBuf);
	virtual ~TSStream();

	bool CheckSign(const unsigned int& Code) ;
	bool ParseOneFrame() ;
	bool CheckIfFrameValid() ; 
};

#endif // !defined(TSSTREAM_H)
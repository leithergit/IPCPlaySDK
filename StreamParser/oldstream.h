#ifndef OLDSTREAM_H
#define OLDSTREAM_H

#include "StreamParser.h"

class OldStream : public StreamParser 
{
public:
	OldStream(unsigned char* rawBuf);
	~OldStream();

	bool CheckSign(const unsigned int& Code) ;
	bool ParseOneFrame() ;
	bool CheckIfFrameValid() ; 
};

#endif /* OLDSTREAM_H */






















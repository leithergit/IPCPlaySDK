#ifndef _SVAC_STREAM_H
#define _SVAC_STREAM_H

#include "StreamParser.h"

class SvacStream : public StreamParser  
{
public:
	SvacStream(unsigned char* rawBuf);
	virtual ~SvacStream();
	
	bool CheckSign(const unsigned int& Code) ;
	bool ParseOneFrame() ;
	bool CheckIfFrameValid() ; 

private:
	unsigned long m_extendLen ;
};

#endif
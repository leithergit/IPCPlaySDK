#ifndef DHSTDSTREAM_H
#define DHSTDSTREAM_H

#include "StreamParser.h"
	
class DhStdStream : public StreamParser  
{
public:
	DhStdStream(unsigned char* rawBuf);
	virtual ~DhStdStream();

	bool CheckSign(const unsigned int& Code) ;
	bool ParseOneFrame() ;
	bool CheckIfFrameValid() ; 
private:
	unsigned long m_frameSeq ;
	unsigned long m_extendLen ;
	bool m_error ;
	int m_encType;
};

#endif 

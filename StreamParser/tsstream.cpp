#include "tsstream.h"

TSStream::TSStream(unsigned char* rawBuf):StreamParser(rawBuf)
{
}

TSStream::~TSStream()
{
}

inline
bool TSStream::CheckSign(const unsigned int& Code)
{
	return false ;
}

inline
bool TSStream::ParseOneFrame()
{
	return false ;
}

inline int log2bin(unsigned int value) 
{
	int n = 0;
	while (value) {
		value>>=1;
		n++;
	}
	return n;
}

#define MMAX(a,b) ((a)>(b)?(a):(b))

inline
bool TSStream::CheckIfFrameValid()
{
	return true ;
}
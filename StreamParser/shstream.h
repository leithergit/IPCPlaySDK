#ifndef SHSTREAM_H
#define SHSTREAM_H

#include "rwstream.h"

#define MAXSHSTRBUFLEN 131072

class ShStream : public StreamParser  
{
public:
	ShStream(unsigned char* rawBuf);
	virtual ~ShStream();

	bool CheckSign(const unsigned int& Code) ;
	bool ParseOneFrame() ;
	bool CheckIfFrameValid() ; 

	int ParseData(unsigned char *data, int datalen) ;	
	DH_FRAME_INFO *GetNextFrame() ;
	DH_FRAME_INFO *GetNextKeyFrame() ;

private:
	unsigned char m_shbuffer[MAXSHSTRBUFLEN] ;
	RwStream* m_rwstream ;
	
	DH_FRAME_INFO* m_ShFrameInfo ;
};
#endif /* SHSTREAM_H */

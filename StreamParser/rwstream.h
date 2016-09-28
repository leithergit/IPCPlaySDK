#ifndef RWSTREAM_H
#define RWSTREAM_H

#include "StreamParser.h"

typedef enum
{
	MPEG_Standand,
	MPEG_SH2,
 	MPEG_AVI
}MPEGTYPE ;

class RwStream : public StreamParser 
{
public:
	RwStream(unsigned char* rawBuf, MPEGTYPE mp4type = MPEG_Standand, DH_FRAME_INFO* AviInfo = NULL);
	~RwStream();
	
	bool CheckSign(const unsigned int& Code) ;
	int ParseData(unsigned char *data, int datalen) ;

	int CheckIfIFrame(unsigned char* buf, int signpos) ;
	void AddFrameInfo(DH_FRAME_INFO* frame_info) ;
	void GetShFrameInfo(const DH_FRAME_INFO* shframeinfo) ;
	void ReSet(int level) ;
private:
	int m_datapos_1 ;
	int m_datapos_2 ;
	MPEGTYPE m_mp4type ;
	long m_lastRemain ;
	bool m_IfFindIFrame ; 
	DH_FRAME_INFO m_ShFrameInfo ;
	DH_FRAME_INFO m_AviInfo;
	int tmp_IFrameHeadLenth;
};

#endif /* RWSTREAM_H */











































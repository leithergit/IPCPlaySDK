// StreamParser.h: interface for the StreamParser class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STREAMPARSER_H__855E8C45_33DB_4F24_AF2A_CA819E6D11E3__INCLUDED_)
#define AFX_STREAMPARSER_H__855E8C45_33DB_4F24_AF2A_CA819E6D11E3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FrameList.h"
#include "DhStreamParser.h"

class StreamParser 
{
public:
	StreamParser(unsigned char* rawBuf) ;
	virtual ~StreamParser();
	
	virtual bool CheckSign(const unsigned int& Code) = 0 ;
	virtual bool ParseOneFrame(){return true ;} ;
	virtual bool CheckIfFrameValid(){return true ;} ;

	virtual int ParseData(unsigned char *data, int datalen) ;	
	virtual DH_FRAME_INFO *GetNextFrame() ;
	virtual DH_FRAME_INFO *GetNextKeyFrame() ;

	virtual int Reset(int level);
	int GetFrameDataListSize();; //chenfeng20090511 -s

public:
	CFrameList<DH_FRAME_INFO> m_FrameInfoList ;//帧信息链表
protected:
	DH_FRAME_INFO* m_FrameInfo ;//帧信息结构
	DH_FRAME_INFO* m_tmp_Frameinfo;//临时帧信息
	////////////////////////////////////////////////////////////
	unsigned char* m_rawBuf;//原始数据缓冲
	unsigned char *m_bufptr;//数据分析指针
	unsigned int    m_code;//标志
    long m_frameLen; // 帧长
    long m_RemainLen; // 剩余数据长度
	long  rest ;//未分析数据
	unsigned long m_preShSeq ;
	//2010-1-16刘阳修改start
	bool m_frameNotCon;//帧不连续
	//2010-1-16刘阳修改end
protected:
	static void AudioInfoOpr(DH_FRAME_INFO* frameinfo, const unsigned char& samplespersecond) ;
};


#endif // !defined(AFX_STREAMPARSER_H__855E8C45_33DB_4F24_AF2A_CA819E6D11E3__INCLUDED_)

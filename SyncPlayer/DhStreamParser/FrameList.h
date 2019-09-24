// FrameList.h: interface for the CFrameList class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FRAMELIST_H__D517C6C9_E430_4122_BE39_DC421AEB3028__INCLUDED_)
#define AFX_FRAMELIST_H__D517C6C9_E430_4122_BE39_DC421AEB3028__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "DhStreamParser.h" //chenfeng20090507 -s

#include <queue>
#define  INITQUEUENUM 100

template <class T>
class CFrameList  
{
public:
	CFrameList();
 	virtual ~CFrameList();

public:
	T* GetFreeNote() ;
	T* GetDataNote() ;
	void AddToDataList(T* t) ;
	void AddToFreeList(T* t) ;
	void Reset() ;
	 //chenfeng20090511 -s
	int GetDataListSize(){return m_datalist.size();};
	  //chenfeng20090511 -e

private:
	T* m_tmpNote ;
	std::queue<T*> m_datalist ;
	std::queue<T*> m_freelist ;

	bool bInit_over; //chenfeng20090507 -s
	unsigned int nFrameCnt_in; //chenfeng20090507 -s
	
	unsigned int nFrameCnt_out; //chenfeng20090507 -s
};

template <class T>
CFrameList<T>::CFrameList()
{
	nFrameCnt_in = 0; //chenfeng20090507 -s
	nFrameCnt_out = 0; //chenfeng20090507 -s

	bInit_over = false; //chenfeng20090507 -

	for (int i = 0 ; i < INITQUEUENUM;  ++i)
	{
		m_tmpNote = new T ;
		memset(m_tmpNote,0,sizeof(T)) ;
		AddToFreeList(m_tmpNote) ;
	}
	bInit_over = true; //chenfeng20090507 -

	m_tmpNote = NULL ;
}

template <class T>
CFrameList<T>::~CFrameList()
{
	while (!m_freelist.empty())
	{
		m_tmpNote = m_freelist.front() ;
		if (m_tmpNote != NULL)
		{
			delete m_tmpNote ;
		}
		m_freelist.pop() ;
	}

	while (!m_datalist.empty())
	{
		m_tmpNote = m_datalist.front() ;
		if (m_tmpNote != NULL)
		{
			delete m_tmpNote ;
		}
		m_datalist.pop() ;
	}

	m_tmpNote = NULL ;
}

template <class T>
T* CFrameList<T>::GetFreeNote()
{
	if (!m_freelist.empty())
	{
		m_tmpNote = m_freelist.front() ;
		m_freelist.pop() ;	
	}
	else
	{
		m_tmpNote = new T ;
		memset(m_tmpNote,0,sizeof(T)) ;
	}

	return m_tmpNote ;
}

template <class T>
T* CFrameList<T>::GetDataNote() 
{
	if (m_datalist.empty())
	{
		return NULL ;
	}

	m_tmpNote = m_datalist.front() ;
	m_datalist.pop() ;


	return m_tmpNote ;
}

template <class T>
void CFrameList<T>::AddToDataList(T* t)
{

	m_datalist.push(t) ;
}

template <class T>
void CFrameList<T>::AddToFreeList(T* t) 
{
	m_freelist.push(t) ;
}

template <class T>
void CFrameList<T>::Reset()
{
	while (!m_datalist.empty())
	{
		m_tmpNote = m_datalist.front() ;
		if (m_tmpNote)
		{
			AddToFreeList(m_tmpNote) ;
		}
		m_datalist.pop() ;
	}

	m_tmpNote = NULL ;
}

#endif // !defined(AFX_FRAMELIST_H__D517C6C9_E430_4122_BE39_DC421AEB3028__INCLUDED_)






















#pragma once
#include <windows.h>
#include "Utility.h"
class CStat
{
	int		nCount;			// 统计总数量
	float	fTotalValue;	// 所有时间和
	float	fAvgValue;		// 平均时间
	float   fMax, fMin;
	int		nObjIndex;
	char    szStatName[128];
	float	*pArray;
	int		nArraySize;
	CStat()
	{
		ZeroMemory(this, sizeof(CStat));
	}
public:
	CStat(char *szName, int nIndex, int nSize = 200)
	{
		ZeroMemory(this, sizeof(CStat));
		nObjIndex = nIndex;
		strcpy(szStatName, szName);
		nArraySize = nSize;
		pArray = new float[nArraySize];
		ZeroMemory(pArray, sizeof(float)*nArraySize);
	}
	CStat(int nIndex, int nSize = 200)
	{
		ZeroMemory(this, sizeof(CStat));
		nObjIndex = nIndex;
		nArraySize = nSize;
		strcpy(szStatName, "TimeStat");
		pArray = new float[nArraySize];
		ZeroMemory(pArray, sizeof(float)*nArraySize);
	}
	~CStat()
	{
		if (pArray)
			delete[]pArray;
	}
	const float GetAvgValue()
	{
		return fAvgValue;
	}
	void Stat(float fValue)
	{
		if (nCount < nArraySize)
		{
			pArray[nCount++] = fValue;
			fTotalValue += fValue;
			if (fMax < fValue)
				fMax = fValue;
			if (fMin == 0.0f ||
				fMin > fValue)
				fMin = fValue;
		}
	}

	bool IsFull()
	{
		return (nCount >= 200);
	}

	void Reset()
	{
		ZeroMemory(this, offsetof(CStat, nObjIndex));
	}

	void OutputStat()
	{
		fAvgValue = fTotalValue / nCount;

		TraceMsgA("%s Obj(%d)(%s)\t nQueueSize = %d\tMaxValue = %.3f\tMinValue = %.3f\tAvgValue = %.3f\n\t", __FUNCTION__, nObjIndex, szStatName, nCount, fMax, fMin, fAvgValue);
		char szText[2048] = { 0 };
		for (int i = 0; i < nCount; i++)
		{
			sprintf(&szText[strlen(szText)], "%.03f\t", pArray[i]);
			if (i % 100 == 0 && i != 0)
				strcat(szText, "\n\t");
		}
		strcat(szText, "\r\n");
		TraceMsgA(szText);
	}
};


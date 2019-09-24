// DavFileManager.cpp : implementation file
//

#include "stdafx.h"
#include "SyncPlayer.h"
#include "DavFileManager.h"
#include "afxdialogex.h"
#include "DHStream.hpp"
#include "DhStreamParser.h"


// CDavFileManager dialog

IMPLEMENT_DYNAMIC(CDavFileManager, CDialogEx)

CDavFileManager::CDavFileManager(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDavFileManager::IDD, pParent)
	, m_tFirstFrame(0)
	, m_bCheckStartTime(FALSE)
{

}

CDavFileManager::~CDavFileManager()
{
}

void CDavFileManager::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST_FILE, m_ctlListFile);
	DDX_Control(pDX, IDC_DATETIMEPICKER, m_ctlDateTimer);
	DDX_DateTimeCtrl(pDX, IDC_DATETIMEPICKER, m_tFirstFrame);
	DDX_Check(pDX, IDC_CHECK_STARTTIME, m_bCheckStartTime);
}


BEGIN_MESSAGE_MAP(CDavFileManager, CDialogEx)
	
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CDavFileManager::OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDC_BUTTON_CUT, &CDavFileManager::OnBnClickedButtonCut)
	ON_BN_CLICKED(IDC_BUTTON_MERGE, &CDavFileManager::OnBnClickedButtonMerge)
	ON_NOTIFY(NM_CLICK, IDC_LIST_FILE, &CDavFileManager::OnNMClickListFile)
END_MESSAGE_MAP()


// CDavFileManager message handlers



BOOL CDavFileManager::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	int nCols = 0;
	m_ctlListFile.SetExtendedStyle(m_ctlListFile.GetExtendedStyle() | LVS_EX_FULLROWSELECT |LVS_EX_CHECKBOXES);
	m_ctlListFile.InsertColumn(nCols++, _T("序号"), LVCFMT_LEFT, 60);
	m_ctlListFile.InsertColumn(nCols++, _T("文件名"), LVCFMT_LEFT, 500);
	m_ctlListFile.InsertColumn(nCols++, _T("起始时间"), LVCFMT_LEFT, 120);
	m_ctlListFile.InsertColumn(nCols++, _T("结束时间"), LVCFMT_LEFT, 120);
	m_ctlListFile.InsertColumn(nCols++, _T("总帧数"), LVCFMT_LEFT, 120);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CDavFileManager::OnBnClickedButtonBrowse()
{
	
	UINT nEditID[] = { IDC_EDIT_FILE1, IDC_EDIT_FILE2, IDC_EDIT_FILE3, IDC_EDIT_FILE4, IDC_EDIT_FILE4 };
	TCHAR szText[1024] = { 0 };
	DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_ALLOWMULTISELECT;
	TCHAR  szFilter[] = _T("Video File(*.dav)|*.dav|Viedo File (*.mp4)|*.mp4|H.264 Video File(*.H264)|*.H264|H.265 Video File(*.H265)|*.H265|All Files (*.*)|*.*||");
	TCHAR szExportLog[MAX_PATH] = { 0 };
	CTime tNow = CTime::GetCurrentTime();

	//////////
	const DWORD numberOfFileNames = 32;//最多允许32个文件
	const DWORD fileNameMaxLength = 1024;
	const DWORD nBufferSize = (numberOfFileNames * fileNameMaxLength) + 1;
	TCHAR* pFileNamesBuffer = new TCHAR[nBufferSize];
	// Initialize beginning and end of buffer.
	pFileNamesBuffer[0] = NULL;//必须的
	pFileNamesBuffer[nBufferSize - 1] = NULL;

	CFileDialog OpenFile(TRUE, _T("*.dav"), _T(""), dwFlags, (LPCTSTR)szFilter);
	OpenFile.m_ofn.lpstrTitle = _T("Please Select the files to play");
	CString strFilePath;
	OpenFile.m_ofn.lpstrFile = pFileNamesBuffer;
	OpenFile.m_ofn.nMaxFile = nBufferSize;
	int nFiles = 0;
	int nFrameBufferSize = 128 * 1024;
	byte *pFileBuffer = new byte[nFrameBufferSize];
	ZeroMemory(pFileBuffer, nFrameBufferSize);
	DhStreamParser* pStreamParser = nullptr;
	if (OpenFile.DoModal() == IDOK)
	{
		m_ctlListFile.DeleteAllItems();
		CWaitCursor Wait;
		CString strFileName;
		CString strFlleWrite;
		POSITION pos = OpenFile.GetStartPosition();
		while (pos != NULL)
		{
			_stprintf_s(szText, 1024, _T("%d"), nFiles + 1);
			m_ctlListFile.InsertItem(nFiles, szText);
			strFileName = OpenFile.GetNextPathName(pos);//返回选定文件文件名// Retrieve file name(s).
			m_ctlListFile.SetItemText(nFiles, 1, strFileName);
			//strFlleWrite.Format(_T("%s_"), strFileName);
			try
			{
				CFile fileRead(strFileName, CFile::modeRead | CFile::shareDenyWrite);
				//CFile FileWrite(strFlleWrite, CFile::modeWrite | CFile::modeCreate);
				int nWidth = 0, nHeight = 0, nFramerate = 0;
				DhStreamParser* pStreamParser = new DhStreamParser();
				int nFrames = 0;
				time_t tFirstFrame = 0;
				do 
				{
					int nReadLength = fileRead.Read(pFileBuffer, nFrameBufferSize);
					if (nReadLength > 0)
					{
						if (!pStreamParser)
							pStreamParser = new DhStreamParser();
						pStreamParser->InputData(pFileBuffer, nReadLength);
						while (true)
						{
							DH_FRAME_INFO *pFrame = pStreamParser->GetNextFrame();
							if (!pFrame)
								break;
							if (pFrame->nType == DH_FRAME_TYPE_VIDEO)
							{
								//FileWrite.Write(pFrame->pHeader, pFrame->nLength);
								nFrames++;
								if (!tFirstFrame)
								{
									tFirstFrame = pFrame->nTimeStamp;
									break;
								}
							}
						}
						if (tFirstFrame)
							break;
					}
					else
						break;
				} while (true);
				CTime tFrame(tFirstFrame);
				m_ctlListFile.SetItemText(nFiles,2,tFrame.Format(_T("%Y-%m-%d %H:%M:%S")));
				m_ctlListFile.SetItemData(nFiles, (DWORD_PTR)tFirstFrame);
				_stprintf_s(szText, 1024, _T("%d"), nFrames);
				m_ctlListFile.SetItemText(nFiles, 4, szText);
			}
			catch (CFileException* e)
			{
				TCHAR szError[1024] = { 0 };
				e->GetErrorMessage(szError, 1024, nullptr);
				TraceMsg(_T("%s Exception:%s.\n"), __FUNCTIONW__, szError);
			}
			
			nFiles++;
			delete pStreamParser;
		}
	}
	delete pFileNamesBuffer;
	delete[]pFileBuffer;
}


void CDavFileManager::OnBnClickedButtonCut()
{
	if (m_ctlListFile.GetSelectedCount() > 1)
	{
		AfxMessageBox(_T("您选中了多个文件,截取操作只能对一个文件进行."));
		return;
	}
	UpdateData();
	CString strNewTime = m_tFirstFrame.Format(_T("%H:%M:%S"));
	CString strSourceFile;
	CString strDestFile;
	POSITION  pos = m_ctlListFile.GetFirstSelectedItemPosition();
	int nFrameBufferSize = 128 * 1024;
	byte *pFileBuffer = new byte[nFrameBufferSize];
	ZeroMemory(pFileBuffer, nFrameBufferSize);
	DhStreamParser* pStreamParser = nullptr;
	CString strText;
	while (pos)
	{
		int nItem = m_ctlListFile.GetNextSelectedItem(pos);
		strSourceFile = m_ctlListFile.GetItemText(nItem, 1);
		strDestFile.Format(_T("%s.cut.dav"), strSourceFile);
		try
		{
			CFile fileRead(strSourceFile, CFile::modeRead | CFile::shareDenyWrite);
			CFile FileWrite(strDestFile, CFile::modeWrite | CFile::modeCreate);
			int nWidth = 0, nHeight = 0, nFramerate = 0;
			DhStreamParser* pStreamParser = new DhStreamParser();
			bool bWrite = false;
			do
			{
				int nReadLength = fileRead.Read(pFileBuffer, nFrameBufferSize);
				if (nReadLength > 0)
				{
					if (!pStreamParser)
						pStreamParser = new DhStreamParser();
					pStreamParser->InputData(pFileBuffer, nReadLength);
					while (true)
					{
						DH_FRAME_INFO *pFrame = pStreamParser->GetNextFrame();
						if (!pFrame)
							break;
// 						if (pFrame->nType == DH_FRAME_TYPE_VIDEO && pFrame->nTimeStamp >= m_tFirstFrame.GetTime())
// 						{
// 							if (bWrite)							
// 								FileWrite.Write(pFrame->pHeader, pFrame->nLength);
// 							else if (pFrame->nSubType == DH_FRAME_TYPE_VIDEO_I_FRAME)
// 							{
// 								bWrite = true;
// 								FileWrite.Write(pFrame->pHeader, pFrame->nLength);
// 							}
// 						}
						if (bWrite)
							FileWrite.Write(pFrame->pHeader, pFrame->nLength);
						else if (pFrame->nType == DH_FRAME_TYPE_VIDEO &&
							pFrame->nTimeStamp >= m_tFirstFrame.GetTime() &&
							pFrame->nSubType == DH_FRAME_TYPE_VIDEO_I_FRAME)
						{
							bWrite = true;
							FileWrite.Write(pFrame->pHeader, pFrame->nLength);
						}
					}
				}
				else
					break;
			} while (true);
			strText.Format(_T("截取内容保存到文件:%s中."), strDestFile);
			SetDlgItemText(IDC_STATIC_STATUS, strText);
		}
		catch (CFileException* e)
		{
			TCHAR szError[1024] = { 0 };
			e->GetErrorMessage(szError, 1024, nullptr);
			TraceMsg(_T("%s Exception:%s.\n"), __FUNCTIONW__, szError);
		}

		delete pStreamParser;
	}
	delete pFileBuffer;

	
}


void CDavFileManager::OnBnClickedButtonMerge()
{
	if (m_ctlListFile.GetSelectedCount() <= 1)
	{
		AfxMessageBox(_T("您只选中了一个文件,合取操作必须对多个文件进行."));
		return;
	}
	UpdateData();
	CString strNewTime = m_tFirstFrame.Format(_T("%H:%M:%S"));
	CString strSourceFile;
	CString strDestFile;
	POSITION  pos = m_ctlListFile.GetFirstSelectedItemPosition();
	int nFrameBufferSize = 256 * 1024;
	byte *pFileBuffer = new byte[nFrameBufferSize];
	ZeroMemory(pFileBuffer, nFrameBufferSize);
	DhStreamParser* pStreamParser = nullptr;
	CString strText;
	bool bFirstFile = true;
	CWaitCursor Wait;
	CFile FileWrite;
	while (pos)
	{
		int nItem = m_ctlListFile.GetNextSelectedItem(pos);
		strSourceFile = m_ctlListFile.GetItemText(nItem, 1);
	
		try
		{
			CFile fileRead(strSourceFile, CFile::modeRead | CFile::shareDenyWrite);
			
			if (bFirstFile)
			{
				strDestFile.Format(_T("%s.Merge.dav"), strSourceFile);
				FileWrite.Open(strDestFile, CFile::modeWrite | CFile::modeCreate);
				bFirstFile = false;
			}
			int nWidth = 0, nHeight = 0, nFramerate = 0;
			DhStreamParser* pStreamParser = new DhStreamParser();
			bool bWrite = false;
			do
			{
				int nReadLength = fileRead.Read(pFileBuffer, nFrameBufferSize);
				if (nReadLength > 0)
				{
					if (!pStreamParser)
						pStreamParser = new DhStreamParser();
					pStreamParser->InputData(pFileBuffer, nReadLength);
					while (true)
					{
						DH_FRAME_INFO *pFrame = pStreamParser->GetNextFrame();
						if (!pFrame)
							break;
						if (bWrite)
							FileWrite.Write(pFrame->pHeader, pFrame->nLength);
						else if (pFrame->nType == DH_FRAME_TYPE_VIDEO &&
							pFrame->nSubType == DH_FRAME_TYPE_VIDEO_I_FRAME )
						{
							if (m_bCheckStartTime )
							{
								if (pFrame->nTimeStamp >= m_tFirstFrame.GetTime())
								{
									bWrite = true;
									FileWrite.Write(pFrame->pHeader, pFrame->nLength);
								}
							}
							else
							{
								bWrite = true;
								FileWrite.Write(pFrame->pHeader, pFrame->nLength);
							}
						}
					}
				}
				else
					break;
			} while (true);
			strText.Format(_T("合文件内容保存到文件:%s中."), strDestFile);
			SetDlgItemText(IDC_STATIC_STATUS, strText);
		}
		catch (CFileException* e)
		{
			TCHAR szError[1024] = { 0 };
			e->GetErrorMessage(szError, 1024, nullptr);
			TraceMsg(_T("%s Exception:%s.\n"), __FUNCTIONW__, szError);
		}

		delete pStreamParser;
	}
	delete pFileBuffer;
}


void CDavFileManager::OnNMClickListFile(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (pNMItemActivate->iItem != -1)
	{
		m_tFirstFrame = (time_t)(DWORD_PTR)m_ctlListFile.GetItemData(pNMItemActivate->iItem);
		UpdateData(FALSE);
	}
	*pResult = 0;
}

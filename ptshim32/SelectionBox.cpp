// SelectionBox.cpp : implementation file
//

#include "stdafx.h"

#include <set>
#include <fstream>
#include <sstream>
#include <string>
#include <tchar.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "ptshim.h"
#include "SelectionBox.h"
using namespace std;
extern string PathOfDll;
string thisDllDirPath();
extern CString TestFile;
// SelectionBox dialog

IMPLEMENT_DYNAMIC(CSelectionBox, CDialog)

CSelectionBox::CSelectionBox(std::set<cPassThruInfo>& connectedList, CWnd* pParent /*=NULL*/)
	: CDialog(CSelectionBox::IDD, pParent),  sel(NULL)
{

}

CSelectionBox::~CSelectionBox()
{
}

void CSelectionBox::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDOK, m_button_ok);
	//DDX_Control(pDX, IDC_LIST1, m_listview);
	//	DDX_Control(pDX, IDC_J2534REGINFO, m_detailtext);
	//	DDX_Control(pDX, IDC_BUTTON1, m_button_config);
	DDX_Control(pDX, IDC_EDIT1, m_logfilename);
	DDX_Control(pDX, IDC_EDIT_TEST, m_testfilename);
	DDX_Control(pDX, IDC_EDIT_MSGLOG, m_msglogfile);
	DDX_Control(pDX, IDC_CHECK1, m_AppendMsgLog);
}


BEGIN_MESSAGE_MAP(CSelectionBox, CDialog)
	ON_BN_CLICKED(IDOK, &CSelectionBox::OnBnClickedOk)
//	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, &CSelectionBox::OnLvnItemchangedList1)
	ON_NOTIFY(HDN_ITEMDBLCLICK, 0, &CSelectionBox::OnHdnItemdblclickList1)
//	ON_BN_CLICKED(IDC_BUTTON1, &CSelectionBox::OnBnClickedConfig)
	ON_BN_CLICKED(IDC_BUTTON2, &CSelectionBox::OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_BROWSE_TEST, &CSelectionBox::OnBnClickedBrowseTest)
	ON_BN_CLICKED(IDC_BUTTON3, &CSelectionBox::OnBnClickedButton3)
END_MESSAGE_MAP()

// SelectionBox message handlers
BOOL CSelectionBox::OnInitDialog()
{
	string line;

	CDialog::OnInitDialog();
	ShowWindow(SW_HIDE);

	TCHAR szPath[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, 0, szPath);

	SYSTEMTIME LocalTime;
	GetLocalTime(&LocalTime);

	CString cstrPath;
	cstrPath.Format(_T("%s\\%s_%04d-%02d-%02d_%02d-%02d-%02d_%04d.txt"), szPath, _T("ShimDLL"), LocalTime.wYear,
		LocalTime.wMonth, LocalTime.wDay, LocalTime.wHour, LocalTime.wMinute, LocalTime.wSecond,
		LocalTime.wMilliseconds);

	m_logfilename.SetWindowText(cstrPath);

	cstrPath.Format(_T("%s\\messagelog.txt"), szPath);
	m_msglogfile.SetWindowText(cstrPath);

	PathOfDll = thisDllDirPath();
	CString iniPath(PathOfDll.c_str());
	cstrPath.Format(_T("%stest.ini"), iniPath);
	ifstream myfile(cstrPath);
	if (getline(myfile, line))
	{
		CString fPath(line.c_str());
		m_testfilename.SetWindowText(fPath);
	}

	if (getline(myfile, line))
	{
		CString fPath(line.c_str());
		m_msglogfile.SetWindowText(fPath);
	}

	if (getline(myfile, line))
	{
		int a = atoi(line.c_str());
		m_AppendMsgLog.SetCheck(a);
	}

//	DoPopulateRegistryListbox();

	ShowWindow(SW_SHOW);
	BringWindowToTop();
	m_button_ok.EnableWindow(true);
	// Return TRUE unless you set focus to a control
	return TRUE;
}

// Callback for sorting the omega ListCtrl. Use the data item to get the corresponding
// class and sort by name. This will make more sense later if it's modified to sort by
// serial number
static int CALLBACK CompareByName(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	cPassThruInfo * item1 = (cPassThruInfo *) lParam1;
	cPassThruInfo * item2 = (cPassThruInfo *) lParam2;

	if (item1->Name < item2->Name)
	{
		return -1;
	}
	else if (item1->Name > item2->Name)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/*
void CSelectionBox::DoPopulateRegistryListbox()
{
	m_listview.DeleteAllItems();			// Clear listbox

	// Insert the omegas into the listbox
	std::set<cPassThruInfo>::iterator i;
	for (i = connectedList.begin(); i != connectedList.end(); i++)
	{
		int index;
		index = m_listview.InsertItem(0, i->Name.c_str());
		m_listview.SetItemData(index, (DWORD_PTR) &(*i));
	}

	// Sort the list by the criteria
	m_listview.SortItems(CompareByName, NULL);
}
*/

void CSelectionBox::OnLvnItemchangedList1(NMHDR *pNMHDR, LRESULT *pResult)
{
}

void ConvertTestFile()
{
	string line;
	//ifstream myfile(PathOfDll + "test.txt");
	ifstream myfile(TestFile);
	int row = 0;
	std::vector<std::string> fileLines;
	std::vector<std::string> msgs;
	std::vector<std::string> resps;
	std::vector<std::string> comments;

	if (myfile)
	{
		if (myfile.is_open())
		{
			while (getline(myfile, line))
			{
				row++;
				fileLines.push_back(line);
			}
			myfile.close();
		}
		string first = fileLines.front();
		if (first.find("=") == std::string::npos)
		{
			//In old format
			int i = 0;
			//std::vector<std::string> tmpRows;
			for (;i < fileLines.size() - 1;)
			{
				string line = fileLines.at(i);
				string msg;
				string resp;
				string comment = "";
				msg = line;
				int c = line.find('/');
				if (c > 1)
				{
					comment =  line.substr(c, line.size() - c);
				}
				if (c != 0)	//Not only comments
				{
					msg = line.substr(0, c); // remove comments
					i++;
					resp = fileLines.at(i);
					c = resp.find('/');
					if (c >= 0)
					{
						resp = resp.substr(0, c); // remove comments
					}
					//string newLine = msg + "=" + resp + " //" + comment;
					BOOL msgExist = false;
					for (int x = 0; x < msgs.size();x++)
					{
						string tmpMsg = msgs.at(x);
						tmpMsg.erase(remove(tmpMsg.begin(), tmpMsg.end(), ' '), tmpMsg.end());
						string tmpMsg2 = msg;
						tmpMsg2.erase(remove(tmpMsg2.begin(), tmpMsg2.end(), ' '), tmpMsg2.end());
						if (tmpMsg == tmpMsg2)
						{
							resps.at(x).append(";" + resp);
							msgExist = true;
							break;
						}					
					}
					if (!msgExist)
					{
						msgs.push_back(msg);
						resps.push_back(resp);
						comments.push_back(comment);
					}
				}
				i++;
			}
			ofstream newfile;
			TestFile.Replace(_T(".txt"), _T("-new.txt"));
			newfile.open(TestFile, ios::out | ios::trunc);
			for (int r = 0;r < msgs.size();r++)
			{
				string newLine = msgs.at(r) + "=" + resps.at(r);
				if (comments.at(r).size() > 1)
				{
					newLine += comments.at(r);
				}
				newfile << newLine + "\n";
			}
			newfile.close();
		}
	}
}


void CSelectionBox::OnBnClickedOk()
{
	m_logfilename.GetWindowText(cstrDebugFile);
	m_testfilename.GetWindowText(cstrTestFile);
	m_msglogfile.GetWindowText(cstrMsgLogFile);
	appendMsgLog = (m_AppendMsgLog.GetCheck() == BST_CHECKED);
	//FILE* fp;
	CString iniPath(PathOfDll.c_str());
	iniPath.Format(_T("%stest.ini"), iniPath);
	TestFile = cstrTestFile;
	//Check if need conversion:
	ConvertTestFile();
	ofstream myfile;
	myfile.open(iniPath, ios::out | ios::trunc);
	myfile << CT2A(TestFile);
	myfile << "\n";
	myfile << CT2A(cstrMsgLogFile);
	myfile << "\n";
	myfile << appendMsgLog;
	myfile << "\n";
	myfile.close();
	//_tfopen_s(&fp, iniPath, _T("w"));
	//fprintf(fp, "%s" ,cstrTestFile.GetString());
	//fclose(fp);
	OnOK();
}


void CSelectionBox::OnHdnItemdblclickList1(NMHDR *pNMHDR, LRESULT *pResult)
{

}


void CSelectionBox::OnBnClickedBrowse()
{
	CString cstrFileName(_T("filename.txt"));
	CString cstrFilter(_T("J2534 Logfile (*.txt)|*.txt|All Files (*.*)|*.*||"));

	m_logfilename.GetWindowText(cstrFileName);

	INT_PTR retval;
	CFileDialog Dlg(FALSE, _T(".txt"), cstrFileName, 0, cstrFilter, 0, 0, 1);
	retval = Dlg.DoModal();
	if (retval != IDCANCEL)
	{
	//	m_logfilename.SetWindowText(Dlg.GetFileName());
	//	string Logfile = Dlg.GetFolderPath(); //  +'\' + Dlg.GetFileName();
		m_logfilename.SetWindowText(Dlg.GetPathName() );
	}
}
 /*
cPassThruInfo * CSelectionBox::GetSelectedPassThru()
{
	return sel;
}
*/
CString CSelectionBox::GetDebugFilename()
{
	return cstrDebugFile;
}

CString CSelectionBox::GetTestFilename()
{
	return cstrTestFile;
}

CString CSelectionBox::GetMsgLogFilename()
{
	return cstrMsgLogFile;
}

BOOL CSelectionBox::GetAppendMsgLog()
{
	return appendMsgLog;
}

void CSelectionBox::OnBnClickedBrowseTest()
{
	CString cstrFileName(_T("test.txt"));
	CString cstrFilter(_T("J2534 response file (*.txt)|*.txt|All Files (*.*)|*.*||"));

	m_testfilename.GetWindowText(cstrTestFile);

	INT_PTR retval;
	CFileDialog Dlg(FALSE, _T(".txt"), cstrFileName, 0, cstrFilter, 0, 0, 1);
	retval = Dlg.DoModal();
	if (retval != IDCANCEL)
	{
		//	m_logfilename.SetWindowText(Dlg.GetFileName());
		//	string Logfile = Dlg.GetFolderPath(); //  +'\' + Dlg.GetFileName();
		m_testfilename.SetWindowText(Dlg.GetPathName());
	}
}


void CSelectionBox::OnBnClickedButton3()
{
	CString cstrFileName(_T("messagelog.txt"));
	CString cstrFilter(_T("messagelog file (*.txt)|*.txt|All Files (*.*)|*.*||"));

	m_msglogfile.GetWindowText(cstrMsgLogFile);

	INT_PTR retval;
	CFileDialog Dlg(FALSE, _T(".txt"), cstrFileName, 0, cstrFilter, 0, 0, 1);
	retval = Dlg.DoModal();
	if (retval != IDCANCEL)
	{
		//	m_logfilename.SetWindowText(Dlg.GetFileName());
		//	string Logfile = Dlg.GetFolderPath(); //  +'\' + Dlg.GetFileName();
		m_msglogfile.SetWindowText(Dlg.GetPathName());
	}
}

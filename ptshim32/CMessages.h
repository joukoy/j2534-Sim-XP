#pragma once
#include "afxwin.h"
#include "afxcmn.h"

#include "resource.h"
#include "shim_loader.h"
#include <set>
#include <string>

using namespace std;
// Messages dialog

class CMessages : public CDialog
{
	DECLARE_DYNAMIC(CMessages)

public:
	CMessages(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CMessages();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG2 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	void OnContextMenu(CWnd* pWnd, CPoint point);
	// DDX/DDV support
	virtual void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CButton mOk;
	CButton mCancel;
	afx_msg void OnBnClickedOk();
	void AppendText(string Txt, COLORREF Color);
//	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	CRichEditCtrl m_Richedit;
	CButton m_Copy;
	CButton m_selectall;
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton1();
};

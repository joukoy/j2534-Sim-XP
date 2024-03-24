#pragma once


// MsgWindow dialog

class MsgWindow : public CDialogEx
{
	DECLARE_DYNAMIC(MsgWindow)

public:
	MsgWindow(CWnd* pParent = nullptr);   // standard constructor
	virtual ~MsgWindow();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG2 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};

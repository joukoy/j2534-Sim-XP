// MsgWindow.cpp : implementation file
//

#include "pch.h"
#include "MsgWindow.h"
#include "afxdialogex.h"


// MsgWindow dialog

IMPLEMENT_DYNAMIC(MsgWindow, CDialogEx)

MsgWindow::MsgWindow(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG2, pParent)
{

}

MsgWindow::~MsgWindow()
{
}

void MsgWindow::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(MsgWindow, CDialogEx)
END_MESSAGE_MAP()


// MsgWindow message handlers

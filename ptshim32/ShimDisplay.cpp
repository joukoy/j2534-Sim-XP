// ShimDisplay.cpp : implementation file
//

#include "pch.h"
#include "resource.h"
#include "ShimDisplay.h"


// ShimDisplay

IMPLEMENT_DYNCREATE(ShimDisplay, CFormView)

ShimDisplay::ShimDisplay()
	: CFormView(IDD_DIALOG2)
{
#ifndef _WIN32_WCE
	EnableActiveAccessibility();
#endif

}

ShimDisplay::~ShimDisplay()
{
}

void ShimDisplay::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(ShimDisplay, CFormView)
END_MESSAGE_MAP()


// ShimDisplay diagnostics

#ifdef _DEBUG
void ShimDisplay::AssertValid() const
{
	CFormView::AssertValid();
}

#ifndef _WIN32_WCE
void ShimDisplay::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif
#endif //_DEBUG


// ShimDisplay message handlers

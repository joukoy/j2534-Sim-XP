#pragma once



// ShimDisplay form view

class ShimDisplay : public CFormView
{
	DECLARE_DYNCREATE(ShimDisplay)

protected:
	ShimDisplay();           // protected constructor used by dynamic creation
	virtual ~ShimDisplay();

public:
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG2 };
#endif
#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};



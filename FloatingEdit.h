#if !defined(AFX_FLOATINGEDIT_H__7DC36E2E_AE1D_11D4_A002_006067718D04__INCLUDED_)
#define AFX_FLOATINGEDIT_H__7DC36E2E_AE1D_11D4_A002_006067718D04__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FloatingEdit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFloatingEdit window

class CFloatingEdit : public CEdit
{
// Construction
public:
	CFloatingEdit();

public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFloatingEdit)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFloatingEdit();

	// Generated message map functions
protected:
	CBrush errorBrush;
	CBrush OKBrush;
	CBrush* Check();
	//{{AFX_MSG(CFloatingEdit)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	afx_msg HBRUSH CtlColor(CDC * dc, UINT id);
	afx_msg BOOL OnChange();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FLOATINGEDIT_H__7DC36E2E_AE1D_11D4_A002_006067718D04__INCLUDED_)

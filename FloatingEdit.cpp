// FloatingEdit.cpp : implementation file
//

#include "stdafx.h"
#include "FloatingEdit.h"
#include "libdevcore/CommonData.h"
#include "StringProcess.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFloatingEdit

CFloatingEdit::CFloatingEdit()
{
	errorBrush.CreateSolidBrush(RGB(255, 128, 128));
	OKBrush.CreateSolidBrush(RGB(192, 255, 192));
}

CFloatingEdit::~CFloatingEdit()
{
	errorBrush.DeleteObject();
	OKBrush.DeleteObject();
}

BEGIN_MESSAGE_MAP(CFloatingEdit, CEdit)
	//{{AFX_MSG_MAP(CFloatingEdit)
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
	ON_WM_CTLCOLOR_REFLECT()
	ON_CONTROL_REFLECT_EX(EN_CHANGE, OnChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFloatingEdit message handlers

/****************************************************************************
*                         CFloatingEdit::Check
* Result: CBrush *
*       The desired drawing brush
* Effect: 
*       Sets the SyntaxValid and ValueValid flags
****************************************************************************/

CBrush * CFloatingEdit::Check()
{
    CString s;
    CEdit::GetWindowText(s);

	std::wstring textLabel = s.GetString();

	if (dev::isHex(WideStringToUTF8(textLabel)))
		return &OKBrush;
	else
		return &errorBrush;

} // CFloatingEdit::Check

/****************************************************************************
*                           CFloatingEdit::CtlColor
* Inputs:
*       CDC * dc: Display context
*	UINT id: code
* Result: HBRUSH
*       Brush handle
* Effect: 
*       Changes the control color based on the computed state
*	    emptyBrush: Control is empty
*	    OKBrush: Data is valid
*	    partialBrush: Data is incomplete
*	    errorBrush: Data is wrong
*	Sets the Valid flag, so the IsValid method will report the
*	current status. The IsValid flag will be TRUE only if the
*	control is "green" (&OKBrush)
* Notes:
*	This parses a floating point number of the form
*		{ '+' | '-' }? <0..9>* { '.' <0..9>+ }?
*                       { 'e' | 'E' { '+' | '-' }? <0..9>+ }?
****************************************************************************/

HBRUSH CFloatingEdit::CtlColor(CDC * dc, UINT id)
    {
     CBrush * brush = Check();
     
     LOGBRUSH br;
     brush->GetLogBrush(&br);
     dc->SetBkColor(br.lbColor);
     return (HBRUSH)*brush;
    } // CFloatingEdit::CtlColor

/****************************************************************************
*                           CFloatingEdit::OnChange
* Result: BOOL
*       TRUE to terminate routing (not used)
*	FALSE to allow routing to continue (only value returned)
* Effect: 
*       Invalidates the entire control so colors come out right. Otherwise
*	the "optimizations" of the redraw will leave the colors banded in
*	odd ways and only update the area around the text, not the entire
*	box.
****************************************************************************/

BOOL CFloatingEdit::OnChange()
    {
     InvalidateRect(NULL);
     return FALSE;
    } // CFloatingEdit::OnChange

/****************************************************************************
*                            CFloatingEdit::OnChar
* Inputs:
*       UINT nChar: Character code
*	UINT nRepCnt: Repeat count
*	UINT nFlags: Flags
* Result: void
*       
* Effect: 
*       If the character is not a valid character for the edit control,
*	reject it with a beep. This avoids the user typing completely
*	nonsensical characters.
****************************************************************************/

void CFloatingEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	switch (nChar)
	{
		case 1:  // ctrl+a
		case 3:  // ctrl+c
		case 22: // ctrl+v
		case 25: // ctrl+y
		case 26: // ctrl+z
		case _T('A'):
		case _T('B'):
		case _T('C'):
		case _T('D'):
		case _T('E'):
		case _T('F'):
		case _T('a'):
		case _T('b'):
		case _T('c'):
		case _T('d'):
		case _T('e'):
		case _T('f'):
		case _T('1'):
		case _T('2'):
		case _T('3'):
		case _T('4'):
		case _T('5'):
		case _T('6'):
		case _T('7'):
		case _T('8'):
		case _T('9'):
		case _T('0'):
		case _T('\b'):
			break;
		default:
			MessageBeep(0);
			return;
	}

    CEdit::OnChar(nChar, nRepCnt, nFlags);
}

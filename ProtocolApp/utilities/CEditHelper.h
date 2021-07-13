/*
* Copyright (c) Franc Morales
* https://www.codeproject.com/articles/13973/editable-listbox-tutorial
* With small edits.
*/
#pragma once
#include <afxwin.h>

#define		WM_APP_ED_EDIT_FINISHED			( WM_APP + 04101 )

class CEditHelper :
    public CEdit
{
	// Construction
public:
	CEditHelper();

	// Implementation
public:
	virtual ~CEditHelper();

	// Generated message map functions
protected:
	//{{AFX_MSG(CEditHelper)
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};


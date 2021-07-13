/*
* Copyright (c) Franc Morales
* https://www.codeproject.com/articles/13973/editable-listbox-tutorial
* With small edits.
*/
#pragma once
#include <afxwin.h>

#define		WM_APP_LB_ITEM_EDITED			( WM_APP + 04100 )

#include "CEditHelper.h"

class CEditableListBox :
    public CListBox
{
	// Construction
public:
	CEditableListBox();

	// Attributes
protected:
	CEditHelper		m_ceEdit;
	int				m_iItemBeingEdited;

	// Operations
protected:
	void	EditStarts();
	void	EditEnds(BOOL bCommitText = TRUE);

public:

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CEditableListBox)
		//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CEditableListBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CEditableListBox)
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	afx_msg LRESULT OnEditFinished(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};


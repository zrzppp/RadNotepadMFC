
// RadNotepadView.h : interface of the CRadNotepadView class
//

#pragma once


class CRadNotepadView : public CScintillaView
{
protected: // create from serialization only
	CRadNotepadView();
	DECLARE_DYNCREATE(CRadNotepadView)

// Attributes
public:
	CRadNotepadDoc* GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

    virtual void OnModified(_Inout_ SCNotification* pSCNotification);
protected:

// Implementation
public:
	virtual ~CRadNotepadView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    void DefineMarker(int marker, int markerType, COLORREF fore, COLORREF back);

// Generated message map functions
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
public:
    virtual void OnInitialUpdate();
};

#ifndef _DEBUG  // debug version in RadNotepadView.cpp
inline CRadNotepadDoc* CRadNotepadView::GetDocument() const
   { return reinterpret_cast<CRadNotepadDoc*>(m_pDocument); }
#endif

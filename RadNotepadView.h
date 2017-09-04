
#pragma once

class CRadNotepadDoc;

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
    void SetLineEndingsMode(int mode);
    CStringW GetTextRange(Sci_CharacterRange cr);
    CStringW GetCurrentWord(BOOL bSelect = FALSE);

// Overrides
public:
	virtual void OnDraw(CDC* pDC) override;
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    virtual void OnInitialUpdate() override;
    virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) override;
    virtual void OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/) override;

    virtual void OnCharAdded(_Inout_ SCNotification* pSCNotification);
    virtual void OnModified(_Inout_ SCNotification* pSCNotification);
    virtual void OnUpdateUI(_Inout_ SCNotification* pSCNotification);
protected:

// Implementation
public:
	virtual ~CRadNotepadView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    const Language* m_pLanguage;
    BOOL m_bHighlightMatchingBraces;
    BOOL m_bAutoIndent;

// Generated message map functions
protected:
    DECLARE_MESSAGE_MAP()
    afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnUpdateLine(CCmdUI* pCmdUI);
    afx_msg void OnUpdateInsert(CCmdUI* pCmdUI);
    afx_msg void OnViewMarker(UINT nID);
    afx_msg void OnUpdateViewMarker(CCmdUI *pCmdUI);
    afx_msg void OnViewWhitespace();
    afx_msg void OnUpdateViewWhitespace(CCmdUI *pCmdUI);
    afx_msg void OnViewEndOfLine();
    afx_msg void OnUpdateViewEndOfLine(CCmdUI *pCmdUI);
    afx_msg void OnViewWordWrap();
    afx_msg void OnUpdateViewWordWrap(CCmdUI *pCmdUI);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnViewUseTabs();
    afx_msg void OnUpdateViewUseTabs(CCmdUI *pCmdUI);
    afx_msg void OnEditToggleBookmark();
    afx_msg void OnEditPreviousBookmark();
    afx_msg void OnEditNextBookmark();
    afx_msg void OnLineEndings(UINT nID);
    afx_msg void OnUpdateLineEndings(CCmdUI *pCmdUI);
    afx_msg void OnEditMakeUppercase();
    afx_msg void OnEditMakeLowercase();
    afx_msg LRESULT OnCheckUpdate(WPARAM wParam, LPARAM lParam);
    afx_msg void OnEditGotoLine();
    afx_msg void OnEditFindPrevious();
    afx_msg void OnEditFindNextCurrentWord();
    afx_msg void OnEditFindPreviousCurrentWord();
    afx_msg void OnEditFindMatchingBrace();
    afx_msg void OnSchemeNone();
    afx_msg void OnUpdateSchemeNone(CCmdUI *pCmdUI);
    afx_msg void OnScheme(UINT nID);
    afx_msg void OnUpdateScheme(CCmdUI *pCmdUI);
};

#ifndef _DEBUG  // debug version in RadNotepadView.cpp
inline CRadNotepadDoc* CRadNotepadView::GetDocument() const
   { return reinterpret_cast<CRadNotepadDoc*>(m_pDocument); }
#endif


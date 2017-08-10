
// RadNotepadDoc.h : interface of the CRadNotepadDoc class
//

#pragma once

enum Encoding
{
    BOM_ANSI,
    BOM_UTF16_LE,
    BOM_UTF16_BE,
    BOM_UTF8,
};

class CRadNotepadDoc : public CScintillaDoc
{
protected: // create from serialization only
	CRadNotepadDoc();
	DECLARE_DYNCREATE(CRadNotepadDoc)

// Attributes
public:

// Operations
public:
    void CheckUpdated();
    void CheckReadOnly();

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// Implementation
public:
	virtual ~CRadNotepadDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    Encoding m_eEncoding;
    FILETIME m_ftWrite;

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
public:
    virtual void SetTitle(LPCTSTR lpszTitle);

    void SyncModified();
    afx_msg void OnFileRevert();
    afx_msg void OnUpdateFileRevert(CCmdUI *pCmdUI);
    afx_msg void OnUpdateFileSave(CCmdUI *pCmdUI);
    afx_msg void OnFileReadOnly();
    afx_msg void OnUpdateFileReadOnly(CCmdUI *pCmdUI);
    afx_msg void OnEncoding(UINT nID);
    afx_msg void OnUpdateEncoding(CCmdUI *pCmdUI);
};

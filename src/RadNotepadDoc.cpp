
// RadNotepadDoc.cpp : implementation of the CRadNotepadDoc class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "RadNotepad.h"
#endif

#include "RadNotepadDoc.h"
#include "RadWaitCursor.h"

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static const Encoding g_EncodingList[] = { BOM_ANSI, BOM_UTF16_LE, BOM_UTF16_BE, BOM_UTF8 };

static BYTE g_BomBytes[][4] = {
    { 0 },  // BOM_ANSI
    { u'\xFF', u'\xFE', 0 }, // BOM_UTF16_LE
    { u'\xFE', u'\xFF', 0 }, // BOM_UTF16_BE
    { u'\xEF', u'\xBB', u'\xBF', 0 }, // BOM_UTF8
};

static void byteswap16(unsigned short* data, size_t count)
{
    for (size_t i = 0; i < count; ++i)
        data[i] = _byteswap_ushort(data[i]);
}

static UINT GetBomLength(Encoding eEncoding)
{
    PBYTE pBomData = g_BomBytes[eEncoding];
    int i = 0;
    for (i = 0; pBomData[i] != 0; ++i)
        ;
    return i;
}

static BOOL IsBom(PBYTE pData, PBYTE pBomData)
{
    int i = 0;
    for (i = 0; pBomData[i] != 0; ++i)
        if (pBomData[i] != pData[i])
            break;
    return (pBomData[i] == 0);
}

static Encoding CheckBom(PBYTE pData)
{
    for (Encoding eEncoding : g_EncodingList)
    {
        if (g_BomBytes[eEncoding][0] != 0 && IsBom(pData, g_BomBytes[eEncoding]))
            return eEncoding;
    }
    return BOM_ANSI;
}

static int GetLineEndingMode(CScintillaCtrl& rCtrl, int nLine, int def)
{
    CString line = rCtrl.GetLine(nLine);
    int mode = def;
    if (line.Right(2) == _T("\r\n"))
        mode = SC_EOL_CRLF;
    else if (line.Right(1) == _T("\n"))
        mode = SC_EOL_LF;
    else if (line.Right(1) == _T("\r"))
        mode = SC_EOL_CR;
    return mode;
}

// CRadNotepadDoc

IMPLEMENT_DYNCREATE(CRadNotepadDoc, CScintillaDoc)

BEGIN_MESSAGE_MAP(CRadNotepadDoc, CScintillaDoc)
    ON_COMMAND(ID_FILE_REVERT, &CRadNotepadDoc::OnFileRevert)
    ON_UPDATE_COMMAND_UI(ID_FILE_REVERT, &CRadNotepadDoc::OnUpdateFileRevert)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, &CRadNotepadDoc::OnUpdateFileSave)
    ON_COMMAND(ID_FILE_READONLY, &CRadNotepadDoc::OnFileReadOnly)
    ON_UPDATE_COMMAND_UI(ID_FILE_READONLY, &CRadNotepadDoc::OnUpdateFileReadOnly)
    ON_COMMAND_RANGE(ID_ENCODING_ANSI, ID_ENCODING_UTF8, &CRadNotepadDoc::OnEncoding)
    ON_UPDATE_COMMAND_UI_RANGE(ID_ENCODING_ANSI, ID_ENCODING_UTF8, &CRadNotepadDoc::OnUpdateEncoding)
END_MESSAGE_MAP()


// CRadNotepadDoc construction/destruction

CRadNotepadDoc::CRadNotepadDoc()
{
}

CRadNotepadDoc::~CRadNotepadDoc()
{
}

void CRadNotepadDoc::CheckUpdated()
{
    if (!GetPathName().IsEmpty())
    {
        FILETIME ftWrite = {};
        {
            CFile rFile;
            if (rFile.Open(GetPathName(), CFile::modeRead | CFile::shareDenyNone))
                GetFileTime(rFile, NULL, NULL, &ftWrite);
        }

        if (ftWrite.dwHighDateTime == 0 && ftWrite.dwLowDateTime == 0)
        {
            m_bModified = TRUE;
            SetTitle(nullptr);
        }
        else if (CompareFileTime(&ftWrite, &m_ftWrite) != 0)
        {
            m_ftWrite = ftWrite;    // The message box causes another activation, causing an infinite loop
            if (/*!IsModified() ||*/ AfxMessageBox(IDS_FILE_REVERT, MB_ICONQUESTION | MB_YESNO) == IDYES)
                OnFileRevert();
            else
            {
                m_bModified = TRUE;
                SetTitle(nullptr);
            }
        }
    }
}

void CRadNotepadDoc::CheckReadOnly()
{
    if (!GetPathName().IsEmpty())
    {
        CScintillaView* pView = GetView();
        CScintillaCtrl& rCtrl = pView->GetCtrl();

        DWORD dwAttr = GetFileAttributes(GetPathName());
        BOOL bReadOnly = pView->GetUseROFileAttributeDuringLoading() && dwAttr != INVALID_FILE_ATTRIBUTES && dwAttr & FILE_ATTRIBUTE_READONLY;
        if (bReadOnly != rCtrl.GetReadOnly())
        {
            rCtrl.SetReadOnly(bReadOnly);
            SetTitle(nullptr);
        }
    }
}

BOOL CRadNotepadDoc::OnNewDocument()
{
	if (!CScintillaDoc::OnNewDocument())
		return FALSE;

    m_eEncoding = theApp.m_Settings.DefaultEncoding;
    m_nLineEndingMode = theApp.m_Settings.DefaultLineEnding;
    m_ftWrite.dwHighDateTime = 0;
    m_ftWrite.dwLowDateTime = 0;

	return TRUE;
}

BOOL CRadNotepadDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    m_eEncoding = theApp.m_Settings.DefaultEncoding;
    if (PathFileExists(lpszPathName))
        return CScintillaDoc::OnOpenDocument(lpszPathName);
    else
        return TRUE; // Allow opening non-existent files
}

// CRadNotepadDoc serialization

static void AddText(CScintillaCtrl& rCtrl, LPBYTE Buffer, int nBytesRead, Encoding eEncoding)
{
    // TODO Check/Handle when nBytesRead is not an exact multiple
    if (nBytesRead > 0)
    {
        switch (eEncoding)
        {
        case BOM_ANSI:
        case BOM_UTF8:
            rCtrl.AddText(nBytesRead / sizeof(char), reinterpret_cast<char*>(Buffer));
            ASSERT(nBytesRead % sizeof(char) == 0);
            break;

        case BOM_UTF16_BE:
            byteswap16(reinterpret_cast<unsigned short*>(Buffer), nBytesRead / sizeof(unsigned short));
            // fallthrough
        case BOM_UTF16_LE:
            rCtrl.AddText(nBytesRead / sizeof(wchar_t), reinterpret_cast<wchar_t*>(Buffer));
            ASSERT(nBytesRead % sizeof(wchar_t) == 0);
            break;

        default:
            ASSERT(FALSE);
            break;
        }
    }
}

static void WriteBom(CFile* pFile, Encoding eEncoding)
{
    UINT nLen = GetBomLength(eEncoding);
    if (nLen > 0)
        pFile->Write(g_BomBytes[eEncoding], GetBomLength(eEncoding));
}

static void WriteText(CFile* pFile, const char* Buffer, int nBytes, Encoding eEncoding)
{
    if (nBytes > 0)
    {
        switch (eEncoding)
        {
        case BOM_ANSI:
        case BOM_UTF8:
            pFile->Write(Buffer, nBytes);
            break;

        case BOM_UTF16_BE:
        case BOM_UTF16_LE:
            {
                CStringW s = CScintillaCtrl::UTF82W(Buffer, nBytes);
                if (eEncoding == BOM_UTF16_BE)
                {
                    int nLen = s.GetLength();
                    byteswap16(reinterpret_cast<unsigned short*>(s.GetBuffer()), nLen);
                    s.ReleaseBuffer(nLen);
                }
                pFile->Write(s, s.GetLength() * sizeof(WCHAR));
            }
            break;

        default:
            ASSERT(FALSE);
            break;
        }
    }
}

void CRadNotepadDoc::Serialize(CArchive& ar)
{
    CScintillaCtrl& rCtrl = GetView()->GetCtrl();
    CRadWaitCursor wc;
    CMFCStatusBar* pMsgWnd = wc.GetStatusBar();
    const int nIndex = 0;
    pMsgWnd->EnablePaneProgressBar(nIndex, 100, TRUE);

    const int BUFSIZE = 4 * 1024;

    if (ar.IsLoading())
    {
        rCtrl.SetRedraw(FALSE);

        //Tell the control not to maintain any undo info while we stream the data
        rCtrl.Cancel();
        rCtrl.SetUndoCollection(FALSE);

        //Read the data in from the file in blocks
        CFile* pFile = ar.GetFile();
        ULONGLONG nLength = pFile->GetLength();
        ULONGLONG nBytesReadTotal = 0;

        BYTE Buffer[BUFSIZE];
        int nBytesRead = 0;
        bool bFirst = true;
        do
        {
            nBytesRead = pFile->Read(Buffer, BUFSIZE);
            if (bFirst)
            {
                ASSERT(nBytesRead >= 3);
                bFirst = false;
                m_eEncoding = CheckBom(Buffer);
                UINT nLen = GetBomLength(m_eEncoding);
                AddText(rCtrl, Buffer + nLen, nBytesRead - nLen, m_eEncoding);
            }
            else
                AddText(rCtrl, Buffer, nBytesRead, m_eEncoding);
            nBytesReadTotal += nBytesRead;
            pMsgWnd->SetPaneProgress(nIndex, static_cast<long>(nBytesReadTotal * 100 / nLength));

            MSG msg;
            while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
            {
                if (!::AfxPumpMessage())
                    break;
            }
        } while (nBytesRead);

        CheckReadOnly();
        m_nLineEndingMode = ::GetLineEndingMode(rCtrl, 0, m_nLineEndingMode);

        //Reinitialize the control settings
        rCtrl.SetUndoCollection(TRUE);
        rCtrl.EmptyUndoBuffer();
        rCtrl.SetSavePoint();
        rCtrl.GotoPos(0);

        rCtrl.SetRedraw(TRUE);
    }
    else
    {
        //Get the length of the document
        int nDocLength = rCtrl.GetLength();
        ULONGLONG nBytesWriteTotal = 0;

        //Write the data in blocks to disk
        CFile* pFile = ar.GetFile();
        WriteBom(pFile, m_eEncoding);
        for (int i = 0; i<nDocLength; i += (BUFSIZE - 1)) // (BUFSIZE - 1) because data will be returned nullptr terminated
        {
            int nGrabSize = nDocLength - i;
            if (nGrabSize > (BUFSIZE - 1))
                nGrabSize = (BUFSIZE - 1);

            //Get the data from the control
            Sci_TextRange tr;
            tr.chrg.cpMin = i;
            tr.chrg.cpMax = i + nGrabSize;
            char Buffer[BUFSIZE];
            tr.lpstrText = Buffer;
            rCtrl.GetTextRange(&tr);

            //Write it to disk
            WriteText(pFile, Buffer, nGrabSize, m_eEncoding);
            nBytesWriteTotal += nGrabSize;
            pMsgWnd->SetPaneProgress(nIndex, static_cast<long>(nBytesWriteTotal * 100 / nDocLength));

            MSG msg;
            while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
            {
                if (!::AfxPumpMessage())
                    break;
            }
        }
    }
    pMsgWnd->EnablePaneProgressBar(nIndex, -1);
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CRadNotepadDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CRadNotepadDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data.
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CRadNotepadDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CRadNotepadDoc diagnostics

#ifdef _DEBUG
void CRadNotepadDoc::AssertValid() const
{
    CScintillaDoc::AssertValid();
}

void CRadNotepadDoc::Dump(CDumpContext& dc) const
{
    CScintillaDoc::Dump(dc);
}
#endif //_DEBUG


// CRadNotepadDoc commands


void CRadNotepadDoc::SetTitle(LPCTSTR lpszTitle)
{
    CScintillaView* pView = GetView();
    CString strTitle = lpszTitle ? lpszTitle : GetTitle();
    strTitle.Remove(_T('^'));
    strTitle.Remove(_T('*'));
    if (pView->GetCtrl().GetReadOnly())
        strTitle += _T('^');
    if (IsModified())
        strTitle += _T('*');

    CScintillaDoc::SetTitle(strTitle);
}

void CRadNotepadDoc::SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU)
{
    bool bPathNameFirstSet = GetPathName().IsEmpty();
    CScintillaDoc::SetPathName(lpszPathName, bAddToMRU && PathFileExists(lpszPathName));

    CFile rFile;
    if (rFile.Open(GetPathName(), CFile::modeRead | CFile::shareDenyNone))
        GetFileTime(rFile, NULL, NULL, &m_ftWrite);

    if (bPathNameFirstSet)
        UpdateAllViews(nullptr, HINT_PATH_UPDATED);
}

CFile* CRadNotepadDoc::GetFile(LPCTSTR lpszFileName, UINT nOpenFlags, CFileException* pError)
{
    if (nOpenFlags & CFile::shareDenyWrite)
    {   // Allow reading locked files
        nOpenFlags &= ~CFile::shareDenyWrite;
        nOpenFlags |= CFile::shareDenyNone;
    }
    return CScintillaDoc::GetFile(lpszFileName, nOpenFlags, pError);
}

void CRadNotepadDoc::SyncModified()
{
    CScintillaView* pView = GetView();
    if (m_bModified != pView->GetCtrl().GetModify())
    {
        m_bModified = pView->GetCtrl().GetModify();
        SetTitle(nullptr);
    }
}


void CRadNotepadDoc::OnFileRevert()
{
    OnOpenDocument(GetPathName());
    UpdateAllViews(nullptr, HINT_REVERT);
}


void CRadNotepadDoc::OnUpdateFileRevert(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(!GetPathName().IsEmpty() && PathFileExists(GetPathName()));
}


void CRadNotepadDoc::OnUpdateFileSave(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(IsModified());
}


void CRadNotepadDoc::OnFileReadOnly()
{
    if (!GetPathName().IsEmpty())
    {
        DWORD dwAttr = GetFileAttributes(GetPathName());
        if (dwAttr != INVALID_FILE_ATTRIBUTES)
        {
            if (dwAttr & FILE_ATTRIBUTE_READONLY)
                dwAttr &= ~FILE_ATTRIBUTE_READONLY;
            else
                dwAttr |= FILE_ATTRIBUTE_READONLY;
            SetFileAttributes(GetPathName(), dwAttr);
            CheckReadOnly();
        }
    }
}


void CRadNotepadDoc::OnUpdateFileReadOnly(CCmdUI *pCmdUI)
{
    CScintillaView* pView = GetView();
    CScintillaCtrl& rCtrl = pView->GetCtrl();
    pCmdUI->Enable(!GetPathName().IsEmpty());
    pCmdUI->SetCheck(rCtrl.GetReadOnly());
}


void CRadNotepadDoc::OnEncoding(UINT nID)
{
    Encoding e = static_cast<Encoding>(nID - ID_ENCODING_ANSI);
    if (m_eEncoding != e)
    {
        m_eEncoding = e;
        m_bModified = TRUE;
        SetTitle(nullptr);
    }
}

void CRadNotepadDoc::OnUpdateEncoding(CCmdUI *pCmdUI)
{
    Encoding e = static_cast<Encoding>(pCmdUI->m_nID - ID_ENCODING_ANSI);
    pCmdUI->SetCheck(e == m_eEncoding);
}
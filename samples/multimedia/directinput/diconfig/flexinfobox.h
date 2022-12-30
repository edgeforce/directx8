//-----------------------------------------------------------------------------
// File: flexinfobox.h
//
// Desc: Implements a simple text box that displays a text string.
//       CFlexInfoBox is derived from CFlexWnd.  It is used by the page
//       for displaying direction throughout the UI.  The strings are
//       stored as resources.  The class has a static buffer which will
//       be filled with the string by the resource API when needed.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __FLEXINFOBOX_H__
#define __FLEXINFOBOX_H__

class CFlexInfoBox : public CFlexWnd
{
	TCHAR m_tszText[MAX_PATH];  // Text string of the message
	int m_iCurIndex;  // Current text index
	COLORREF m_rgbText, m_rgbBk, m_rgbSelText, m_rgbSelBk, m_rgbFill, m_rgbLine;
	HFONT m_hFont;

	void SetRect();
	void InternalPaint(HDC hDC);

	RECT GetRect(const RECT &);
	RECT GetRect();

public:
	CFlexInfoBox();
	virtual ~CFlexInfoBox();

	void SetText(int iIndex);

	// cosmetics
	void SetFont(HFONT hFont);
	void SetColors(COLORREF text, COLORREF bk, COLORREF seltext, COLORREF selbk, COLORREF fill, COLORREF line);

	virtual void OnPaint(HDC hDC);
};

#endif

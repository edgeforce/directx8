//-----------------------------------------------------------------------------
// File: flexinfobox.cpp
//
// Desc: Implements a simple text box that displays a text string.
//       CFlexInfoBox is derived from CFlexWnd.  It is used by the page
//       for displaying direction throughout the UI.  The strings are
//       stored as resources.  The class has a static buffer which will
//       be filled with the string by the resource API when needed.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"

CFlexInfoBox::CFlexInfoBox() :
	m_iCurIndex(-1),
	m_rgbText(RGB(255,255,255)),
	m_rgbBk(RGB(0,0,0)),
	m_rgbSelText(RGB(0,0,255)),
	m_rgbSelBk(RGB(0,0,0)),
	m_rgbFill(RGB(0,0,255)),
	m_rgbLine(RGB(0,255,255)),
	m_hFont(NULL)
{
}

CFlexInfoBox::~CFlexInfoBox()
{
}

void CFlexInfoBox::SetText(int iIndex)
{
	if (iIndex == m_iCurIndex)
		return;

	// Load the string from resource
	LoadString(g_hModule, iIndex, m_tszText, MAX_PATH);
	DWORD dwe = GetLastError();

	m_iCurIndex = iIndex;
	Invalidate();
}

void CFlexInfoBox::SetFont(HFONT hFont)
{
	m_hFont = hFont;

	if (m_hWnd == NULL)
		return;

	Invalidate();
}

void CFlexInfoBox::SetColors(COLORREF text, COLORREF bk, COLORREF seltext, COLORREF selbk, COLORREF fill, COLORREF line)
{
	m_rgbText = text;
	m_rgbBk = bk;
	m_rgbSelText = seltext;
	m_rgbSelBk = selbk;
	m_rgbFill = fill;
	m_rgbLine = line;
	Invalidate();
}

void CFlexInfoBox::SetRect()
{
	if (m_hWnd == NULL)
		return;

	RECT rect = GetRect();
	SetWindowPos(m_hWnd, NULL, rect.left, rect.top, rect.right, rect.bottom, SWP_NOZORDER | SWP_NOMOVE);
}

RECT CFlexInfoBox::GetRect(const RECT &rect)
{
	int h = GetTextHeight(m_hFont);
	RECT ret = {rect.left, rect.top, rect.right, rect.top + h + 2};
	return ret;
}

RECT CFlexInfoBox::GetRect()
{
	RECT rect;
	GetClientRect(&rect);
	return GetRect(rect);
}

void CFlexInfoBox::OnPaint(HDC hDC)
{
	HDC hBDC = NULL, hODC = NULL;
	CBitmap *pbm = NULL;

	if (!InRenderMode())
	{
		hODC = hDC;
		pbm = CBitmap::Create(GetClientSize(), RGB(0,0,0), hDC);
		if (pbm != NULL)
		{
			hBDC = pbm->BeginPaintInto();
			if (hBDC != NULL)
			{
				hDC = hBDC;
			}
		}
	}

	InternalPaint(hDC);

	if (!InRenderMode())
	{
		if (pbm != NULL)
		{
			if (hBDC != NULL)
			{
				pbm->EndPaintInto(hBDC);
				pbm->Draw(hODC);
			}
			delete pbm;
		}
	}
}

void CFlexInfoBox::InternalPaint(HDC hDC)
{
	TCHAR tszResourceString[MAX_PATH];
	HGDIOBJ hPen = (HGDIOBJ)CreatePen(PS_SOLID, 1, m_rgbLine);
	if (hPen != NULL)
	{
		HGDIOBJ hOldPen = SelectObject(hDC, hPen);

		HGDIOBJ hBrush = (HGDIOBJ)CreateSolidBrush(m_rgbBk);
		if (hBrush != NULL)
		{
			HGDIOBJ hOldBrush = SelectObject(hDC, hBrush);

			RECT rect = {0,0,0,0}, titlerc;
			GetClientRect(&rect);
			titlerc = rect;
			Rectangle(hDC, rect.left, rect.top, rect.right, rect.bottom);

			rect.left++;
			rect.top++;
			rect.right--;
			rect.bottom--;

			SetTextColor(hDC, m_rgbLine);  // User line color for Information title
			SetBkMode(hDC, TRANSPARENT);

			LoadString(g_hModule, IDS_INFO_TITLE, tszResourceString, MAX_PATH);
			// Get the title area rantangle
			DrawText(hDC, tszResourceString, -1, &titlerc, DT_CENTER|DT_NOPREFIX|DT_CALCRECT);
			// Adjust right edge position to old value
			titlerc.right = rect.right + 1;
			// Draw a rectangle around the title area.
			Rectangle(hDC, titlerc.left, titlerc.top, titlerc.right, titlerc.bottom);
			// Draw title text (Information)
			DrawText(hDC, tszResourceString, -1, &titlerc, DT_CENTER|DT_NOPREFIX);
			// Draw the message text
			SetTextColor(hDC, m_rgbText);
			rect.top = titlerc.bottom + 1;
			DrawText(hDC, m_tszText, -1, &rect, DT_LEFT|DT_NOPREFIX|DT_WORDBREAK);

			SelectObject(hDC, hOldBrush);
			DeleteObject(hBrush);
		}

		SelectObject(hDC, hOldPen);
		DeleteObject(hPen);
	}
}

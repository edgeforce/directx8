//-----------------------------------------------------------------------------
// File: cdeviceviewtext.cpp
//
// Desc: CDeviceViewText is a class representing a text string in the view
//       window. It is used when the view type is a list view.  CDeviceViewText
//       will print the text of the control name, while CDeviceControl will
//       print the text of the action assigned to that control.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"


CDeviceViewText::CDeviceViewText(CDeviceUI &ui, CDeviceView &view) :
	m_ui(ui), m_view(view),
	m_hFont(NULL),
	m_rgbText(RGB(255,255,255)),
	m_rgbBk(RGB(0,0,0)),
	m_bWrap(FALSE),
	m_ptszText(NULL)
{
	m_rect.left = m_rect.top = m_rect.right = m_rect.bottom = 0;
}

CDeviceViewText::~CDeviceViewText()
{
	if (m_ptszText)
		free(m_ptszText);
	m_ptszText = NULL;
}

void CDeviceViewText::SetLook(HFONT a, COLORREF b, COLORREF c)
{
	m_hFont = a;
	m_rgbText = b;
	m_rgbBk = c;

	Invalidate();
}

void CDeviceViewText::SetPosition(int x, int y)
{
	int w = m_rect.right - m_rect.left;
	int h = m_rect.bottom - m_rect.top;

	m_rect.left = x;
	m_rect.right = x + w;

	m_rect.top = y;
	m_rect.bottom = y + h;

	Invalidate();
}

void CDeviceViewText::SetRect(const RECT &r)
{
	m_rect = r;
	Invalidate();
}

void CDeviceViewText::_SetText(LPCTSTR t)
{
	if (m_ptszText)
		free(m_ptszText);
	if (t)
		m_ptszText = AllocLPTSTR(t);
}

void CDeviceViewText::SetText(LPCTSTR t)
{
	_SetText(t);
	Invalidate(TRUE);
}

void CDeviceViewText::SetTextAndResizeTo(LPCTSTR t)
{
	_SetText(t);
	SIZE s = GetTextSize(m_ptszText, m_hFont);
	m_rect.right = m_rect.left + s.cx;
	m_rect.bottom = m_rect.top + s.cy;
	Invalidate(TRUE);
}

void CDeviceViewText::SetTextAndResizeToWrapped(LPCTSTR t)
{
	_SetText(t);
	if (!m_ptszText)
	{
		m_rect.right = m_rect.left;
		m_rect.bottom = m_rect.top;
		Invalidate(TRUE);
		return;
	}
	RECT rect = {m_rect.left, m_rect.top, g_sizeImage.cx, m_rect.top + 1};
	HDC hDC = CreateCompatibleDC(NULL);
	if (hDC != NULL)
	{
		HGDIOBJ hOld = NULL;
		if (m_hFont)
			hOld = SelectObject(hDC, m_hFont);
		DrawText(hDC, m_ptszText, -1, &rect, DT_CALCRECT | DT_NOPREFIX | DT_WORDBREAK);
		if (m_hFont)
			SelectObject(hDC, hOld);
		DeleteDC(hDC);
	}
	m_rect = rect;
	m_bWrap = TRUE;
	Invalidate(TRUE);
}

void CDeviceViewText::SetWrap(BOOL bWrap)
{
	m_bWrap = bWrap;
	Invalidate();
}

void CDeviceViewText::Invalidate(BOOL bForce)
{
	if (m_ptszText || bForce)
		m_view.Invalidate();
}

void CDeviceViewText::OnPaint(HDC hDC)
{
	if (!m_ptszText)
		return;

	SetTextColor(hDC, m_rgbText);
	SetBkColor(hDC, m_rgbBk);
	SetBkMode(hDC, OPAQUE);
	RECT rect = m_rect;
	HGDIOBJ hOld = NULL;
	if (m_hFont)
		hOld = SelectObject(hDC, m_hFont);
	DrawText(hDC, m_ptszText, -1, &rect, DT_NOPREFIX | (m_bWrap ? DT_WORDBREAK : 0) | DT_RIGHT);
	if (m_hFont)
		SelectObject(hDC, hOld);
}

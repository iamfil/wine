/*
 * Toolbar control
 *
 * Copyright 1998 Eric Kohl
 *
 * NOTES
 *   PLEASE don't try to improve or change this code right now. Many
 *   features are still missing, but I'm working on it. I want to avoid
 *   any confusion. This note will be removed as soon as most of the
 *   features are implemented.
 *     Eric <ekohl@abo.rhein-zeitung.de>
 *
 * TODO:
 *   - Bitmap drawing.
 *   - Button wrapping.
 *   - Messages.
 *   - Notifications.
 *   - Fix TB_GETBITMAPFLAGS.
 *   - Fix TB_GETROWS and TB_SETROWS.
 *   - Tooltip support (partially).
 *   - Unicode suppport.
 *   - Internal COMMCTL32 bitmaps.
 *   - Fix TOOLBAR_Customize. (Customize dialog.)
 *
 * Testing:
 *   - Run tests using Waite Group Windows95 API Bible Volume 2.
 *     The second cdrom contains executables addstr.exe, btncount.exe,
 *     btnstate.exe, butstrsz.exe, chkbtn.exe, chngbmp.exe, customiz.exe,
 *     enablebtn.exe, getbmp.exe, getbtn.exe, getflags.exe, hidebtn.exe,
 *     indetbtn.exe, insbtn.exe, pressbtn.exe, setbtnsz.exe, setcmdid.exe,
 *     setparnt.exe, setrows.exe, toolwnd.exe.
 *   - additional features.
 */

#include "windows.h"
#include "commctrl.h"
#include "cache.h"
#include "toolbar.h"
#include "heap.h"
#include "win.h"
#include "debug.h"


#define SEPARATOR_WIDTH    8
#define SEPARATOR_HEIGHT   5
#define TOP_BORDER         2
#define BOTTOM_BORDER      2



#define TOOLBAR_GetInfoPtr(wndPtr) ((TOOLBAR_INFO *)wndPtr->wExtra[0])


static void
TOOLBAR_DrawFlatSeparator (LPRECT32 lpRect, HDC32 hdc)
{
    INT32 x = (lpRect->left + lpRect->right) / 2 - 1;
    INT32 yBottom = lpRect->bottom - 3;
    INT32 yTop = lpRect->top + 1;

    SelectObject32 ( hdc, GetSysColorPen32 (COLOR_3DSHADOW));
    MoveToEx32 (hdc, x, yBottom, NULL);
    LineTo32 (hdc, x, yTop);
    x++;
    SelectObject32 ( hdc, GetSysColorPen32 (COLOR_3DHILIGHT));
    MoveToEx32 (hdc, x, yBottom, NULL);
    LineTo32 (hdc, x, yTop);
}


static void
TOOLBAR_DrawButton (WND *wndPtr, TBUTTON_INFO *btnPtr, HDC32 hdc)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    BOOL32 bFlat = (wndPtr->dwStyle & TBSTYLE_FLAT);
    RECT32 rc;

    if (btnPtr->fsState & TBSTATE_HIDDEN) return;

    rc = btnPtr->rect;
    if (btnPtr->fsStyle & TBSTYLE_SEP) {
	if ((bFlat) && (btnPtr->idCommand == 0))
	    TOOLBAR_DrawFlatSeparator (&btnPtr->rect, hdc);
	return;
    }

    /* disabled */
    if (!(btnPtr->fsState & TBSTATE_ENABLED)) {
	HICON32 hIcon;
	DrawEdge32 (hdc, &rc, EDGE_RAISED,
		    BF_SOFT | BF_RECT | BF_MIDDLE | BF_ADJUST);

//	ImageList_Draw (infoPtr->himlDis, btnPtr->iBitmap, hdc,
//			rc.left+1, rc.top+1, ILD_NORMAL);
	return;
    }

    /* pressed TBSTYLE_BUTTON */
    if (btnPtr->fsState & TBSTATE_PRESSED) {
	DrawEdge32 (hdc, &rc, EDGE_SUNKEN,
		    BF_RECT | BF_MIDDLE | BF_ADJUST);
	ImageList_Draw (infoPtr->himlDef, btnPtr->iBitmap, hdc,
			rc.left+2, rc.top+2, ILD_NORMAL);
	return;
    }

    /* checked TBSTYLE_CHECK*/
    if ((btnPtr->fsStyle & TBSTYLE_CHECK) &&
	(btnPtr->fsState & TBSTATE_CHECKED)) {
	HBRUSH32 hbr;
	DrawEdge32 (hdc, &rc, EDGE_SUNKEN,
		    BF_RECT | BF_MIDDLE | BF_ADJUST);

	hbr = SelectObject32 (hdc, CACHE_GetPattern55AABrush ());
	PatBlt32 (hdc, rc.left, rc.top, rc.right - rc.left,
		  rc.bottom - rc.top, 0x00FA0089);
	SelectObject32 (hdc, hbr);
	ImageList_Draw (infoPtr->himlDef, btnPtr->iBitmap, hdc,
			rc.left+2, rc.top+2, ILD_NORMAL);
	return;
    }

    /* indeterminate */	
    if (btnPtr->fsState & TBSTATE_INDETERMINATE) {
	HBRUSH32 hbr;
	DrawEdge32 (hdc, &rc, EDGE_RAISED,
		    BF_SOFT | BF_RECT | BF_MIDDLE | BF_ADJUST);

	hbr = SelectObject32 (hdc, CACHE_GetPattern55AABrush ());
	PatBlt32 (hdc, rc.left, rc.top, rc.right - rc.left,
		  rc.bottom - rc.top, 0x00FA0089);
	SelectObject32 (hdc, hbr);
//	ImageList_Draw (infoPtr->himlDis, btnPtr->iBitmap, hdc,
//			rc.left+1, rc.top+1, ILD_NORMAL);
	return;
    }

    /* normal state */
    DrawEdge32 (hdc, &rc, EDGE_RAISED,
		BF_SOFT | BF_RECT | BF_MIDDLE | BF_ADJUST);
    ImageList_Draw (infoPtr->himlDef, btnPtr->iBitmap, hdc,
		    rc.left+1, rc.top+1, ILD_NORMAL);
}



static void
TOOLBAR_Refresh (WND *wndPtr, HDC32 hdc)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    INT32 i;

    /* draw buttons */
    btnPtr = infoPtr->buttons;
    for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++)
	TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
}


static void
TOOLBAR_CalcToolbar (WND *wndPtr)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    INT32 i, j, nRows;
    INT32 x, y, cx, cy;
    BOOL32 bVertical;

    x  = infoPtr->nIndent;
    y  = TOP_BORDER;
    cx = infoPtr->nButtonWidth;
    cy = infoPtr->nButtonHeight;
    nRows = 1;

    btnPtr = infoPtr->buttons;
    for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++) {
	bVertical = FALSE;

	if (btnPtr->fsState & TBSTATE_HIDDEN)
	    continue;

	if (btnPtr->fsStyle & TBSTYLE_SEP) {
	    /* UNDOCUMENTED: If a separator has a non zero bitmap index, */
	    /* it is the actual width of the separator. This is used for */
	    /* custom controls in toolbars.                              */
	    if ((wndPtr->dwStyle & TBSTYLE_WRAPABLE) &&
		(btnPtr->fsState & TBSTATE_WRAP)) {
		x = 0;
		y += cy;
		cx = infoPtr->nWidth;
		cy = ((btnPtr->iBitmap == 0) ?
		    SEPARATOR_WIDTH : btnPtr->iBitmap) * 2 / 3;
		nRows++;
		bVertical = TRUE;
	    }
	    else
		cx = (btnPtr->iBitmap == 0) ?
		    SEPARATOR_WIDTH : btnPtr->iBitmap;
	}
	else {
	    /* this must be a button */
	    cx = infoPtr->nButtonWidth;

	}

	btnPtr->rect.left   = x;
	btnPtr->rect.top    = y;
	btnPtr->rect.right  = x + cx;
	btnPtr->rect.bottom = y + cy;

	if (bVertical) {
	    x = 0;
	    y += cy;
	    if (i < infoPtr->nNumButtons)
		nRows++;
	}
	else
	    x += cx;
    }

    infoPtr->nHeight = y + cy + BOTTOM_BORDER;
}


static INT32
TOOLBAR_InternalHitTest (WND *wndPtr, LPPOINT32 lpPt)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    INT32 i;
    
    btnPtr = infoPtr->buttons;
    for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++) {
	if (btnPtr->fsStyle & TBSTYLE_SEP) {
	    if (PtInRect32 (&btnPtr->rect, *lpPt)) {
		TRACE (toolbar, " ON SEPARATOR %d!\n", i);
		return -i;
	    }
	}
	else {
	    if (PtInRect32 (&btnPtr->rect, *lpPt)) {
		TRACE (toolbar, " ON BUTTON %d!\n", i);
		return i;
	    }
	}
    }

    TRACE (toolbar, " NOWHERE!\n");
    return -1;
}


static INT32
TOOLBAR_GetButtonIndex (TOOLBAR_INFO *infoPtr, INT32 idCommand)
{
    TBUTTON_INFO *btnPtr;
    INT32 i;

    btnPtr = infoPtr->buttons;
    for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++) {
	if (btnPtr->idCommand == idCommand) {
	    TRACE (toolbar, "command=%d index=%d\n", idCommand, i);
	    return i;
	}
    }
    TRACE (toolbar, "no index found for command=%d\n", idCommand);
    return -1;
}


static INT32
TOOLBAR_GetCheckedGroupButtonIndex (TOOLBAR_INFO *infoPtr, INT32 nIndex)
{
    TBUTTON_INFO *btnPtr;
    INT32 nRunIndex;

    if ((nIndex < 0) || (nIndex > infoPtr->nNumButtons))
	return -1;

    /* check index button */
    btnPtr = &infoPtr->buttons[nIndex];
    if ((btnPtr->fsStyle & TBSTYLE_CHECKGROUP) == TBSTYLE_CHECKGROUP) {
	if (btnPtr->fsState & TBSTATE_CHECKED)
	    return nIndex;
    }

    /* check previous buttons */
    nRunIndex = nIndex - 1;
    while (nRunIndex >= 0) {
	btnPtr = &infoPtr->buttons[nRunIndex];
	if ((btnPtr->fsStyle & TBSTYLE_CHECKGROUP) == TBSTYLE_CHECKGROUP) {
	    if (btnPtr->fsState & TBSTATE_CHECKED)
		return nRunIndex;
	}
	else
	    break;
	nRunIndex--;
    }

    /* check next buttons */
    nRunIndex = nIndex + 1;
    while (nRunIndex < infoPtr->nNumButtons) {
	btnPtr = &infoPtr->buttons[nRunIndex];	
	if ((btnPtr->fsStyle & TBSTYLE_CHECKGROUP) == TBSTYLE_CHECKGROUP) {
	    if (btnPtr->fsState & TBSTATE_CHECKED)
		return nRunIndex;
	}
	else
	    break;
	nRunIndex++;
    }

    return -1;
}


static LRESULT
TOOLBAR_AddBitmap (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    LPTBADDBITMAP lpAddBmp = (LPTBADDBITMAP)lParam;
    INT32 nIndex = 0;

    if ((!lpAddBmp) || ((INT32)wParam <= 0))
	return -1;

    TRACE (toolbar, "adding %d bitmaps!\n", wParam);

    if (!(infoPtr->himlDef)) {
	/* create new default image list */
	TRACE (toolbar, "creating default image list!\n");
	infoPtr->himlDef =
	    ImageList_Create (infoPtr->nBitmapWidth, infoPtr->nBitmapHeight,
			      ILC_COLOR | ILC_MASK, (INT32)wParam, 2);
    }

    /* Add bitmaps to the default image list */
    if (lpAddBmp->hInst == (HINSTANCE32)0) {
	nIndex = 
	    ImageList_AddMasked (infoPtr->himlDef, (HBITMAP32)lpAddBmp->nID,
				 GetSysColor32 (COLOR_3DFACE));
    }
    else if (lpAddBmp->hInst == HINST_COMMCTRL) {
	/* add internal bitmaps */
	FIXME (toolbar, "internal bitmaps not supported!\n");

	/* Hack to "add" some reserved images within the image list 
	   to get the right image indices */
	nIndex = ImageList_GetImageCount (infoPtr->himlDef);
	ImageList_SetImageCount (infoPtr->himlDef, nIndex + (INT32)wParam);
    }
    else {
	HBITMAP32 hBmp =
	    LoadBitmap32A (lpAddBmp->hInst, (LPSTR)lpAddBmp->nID);

	nIndex = ImageList_Add (infoPtr->himlDef, hBmp, (HBITMAP32)0);

	DeleteObject32 (hBmp); 
    }


    infoPtr->nNumBitmaps += (INT32)wParam;

    return nIndex;
}


static LRESULT
TOOLBAR_AddButtons32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    LPTBBUTTON lpTbb = (LPTBBUTTON)lParam;
    INT32 nOldButtons, nNewButtons, nAddButtons, nCount;
    HDC32 hdc;

    TRACE (toolbar, "adding %d buttons!\n", wParam);

    nAddButtons = (UINT32)wParam;
    nOldButtons = infoPtr->nNumButtons;
    nNewButtons = nOldButtons + nAddButtons;

    if (infoPtr->nNumButtons == 0) {
	infoPtr->buttons =
	    HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
		       sizeof (TBUTTON_INFO) * nNewButtons);
    }
    else {
	TBUTTON_INFO *oldButtons = infoPtr->buttons;
	infoPtr->buttons =
	    HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
		       sizeof (TBUTTON_INFO) * nNewButtons);
	memcpy (&infoPtr->buttons[0], &oldButtons[0],
		nOldButtons * sizeof(TBUTTON_INFO));
        HeapFree (GetProcessHeap (), 0, oldButtons);
    }

    infoPtr->nNumButtons = nNewButtons;

    /* insert new button data (bad implementation)*/
    for (nCount = 0; nCount < nAddButtons; nCount++) {
	infoPtr->buttons[nOldButtons+nCount].iBitmap   = lpTbb[nCount].iBitmap;
	infoPtr->buttons[nOldButtons+nCount].idCommand = lpTbb[nCount].idCommand;
	infoPtr->buttons[nOldButtons+nCount].fsState   = lpTbb[nCount].fsState;
	infoPtr->buttons[nOldButtons+nCount].fsStyle   = lpTbb[nCount].fsStyle;
	infoPtr->buttons[nOldButtons+nCount].dwData    = lpTbb[nCount].dwData;
	infoPtr->buttons[nOldButtons+nCount].iString   = lpTbb[nCount].iString;
    }

    TOOLBAR_CalcToolbar (wndPtr);

    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


// << TOOLBAR_AddButtons32W >>


static LRESULT
TOOLBAR_AddString32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    if (wParam) {
	char szString[256];
	INT32 len;
	TRACE (toolbar, "adding string from resource!\n");

	len = LoadString32A ((HINSTANCE32)wParam, (UINT32)lParam,
			     szString, 256);

	TRACE (toolbar, "len=%d\n", len);
	nIndex = infoPtr->nNumStrings;
	if (infoPtr->nNumStrings == 0) {
	    infoPtr->strings =
		HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(char *));
	}
	else {
	    char **oldStrings = infoPtr->strings;
	    infoPtr->strings =
		HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
			   sizeof(char *) * (infoPtr->nNumStrings + 1));
	    memcpy (&infoPtr->strings[0], &oldStrings[0],
		    sizeof(char *) * infoPtr->nNumStrings);
	    HeapFree (GetProcessHeap (), 0, oldStrings);
	}

	infoPtr->strings[infoPtr->nNumStrings] =
	    HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(char)*(len+1));
	lstrcpy32A (infoPtr->strings[infoPtr->nNumStrings], szString);
	infoPtr->nNumStrings++;
    }
    else {
	char *p = (char*)lParam;
	INT32 len;

	if (p == NULL) return -1;
	TRACE (toolbar, "adding string(s) from array!\n");
	nIndex = infoPtr->nNumStrings;
	while (*p) {
	    len = lstrlen32A (p);
	    TRACE (toolbar, "len=%d\n", len);

	    if (infoPtr->nNumStrings == 0) {
		infoPtr->strings =
		    HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(char *));
	    }
	    else {
		char **oldStrings = infoPtr->strings;
		infoPtr->strings =
		    HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
			       sizeof(char *) * (infoPtr->nNumStrings + 1));
		memcpy (&infoPtr->strings[0], &oldStrings[0],
			sizeof(char *) * infoPtr->nNumStrings);
		HeapFree (GetProcessHeap (), 0, oldStrings);
	    }

	    infoPtr->strings[infoPtr->nNumStrings] =
		HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(char)*(len+1));
	    lstrcpy32A (infoPtr->strings[infoPtr->nNumStrings], p);
	    infoPtr->nNumStrings++;

	    p += (len+1);
	}

    }

    return nIndex;
}


// << TOOLBAR_AddString32W >>


static LRESULT
TOOLBAR_AutoSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    TRACE (toolbar, "auto size!\n");

    return 0;
}


static LRESULT
TOOLBAR_ButtonCount (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    return infoPtr->nNumButtons;
}


static LRESULT
TOOLBAR_ButtonStructSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    if (infoPtr == NULL) {
	ERR (toolbar, "(0x%08lx, 0x%08x, 0x%08lx)\n", (DWORD)wndPtr, wParam, lParam);
	ERR (toolbar, "infoPtr == NULL!\n");
	return 0;
    }

    infoPtr->dwStructSize = (DWORD)wParam;

    return 0;
}


static LRESULT
TOOLBAR_ChangeBitmap (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    HDC32 hdc;
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    btnPtr->iBitmap = LOWORD(lParam);

    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


static LRESULT
TOOLBAR_CheckButton (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    HDC32 hdc;
    INT32 nIndex;
    INT32 nOldIndex = -1;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];

    if (!(btnPtr->fsStyle & TBSTYLE_CHECK))
	return FALSE;

    if (LOWORD(lParam) == FALSE)
	btnPtr->fsState &= ~TBSTATE_CHECKED;
    else {
	if (btnPtr->fsStyle & TBSTYLE_GROUP) {
	    nOldIndex = 
		TOOLBAR_GetCheckedGroupButtonIndex (infoPtr, nIndex);
	    if (nOldIndex == nIndex)
		return 0;
	    if (nOldIndex != -1)
		infoPtr->buttons[nOldIndex].fsState &= ~TBSTATE_CHECKED;
	}
	btnPtr->fsState |= TBSTATE_CHECKED;
    }

    hdc = GetDC32 (wndPtr->hwndSelf);
    if (nOldIndex != -1)
	TOOLBAR_DrawButton (wndPtr, &infoPtr->buttons[nOldIndex], hdc);
    TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    /* FIXME: Send a WM_COMMAND or WM_NOTIFY */

    return TRUE;
}


static LRESULT
TOOLBAR_CommandToIndex (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    return TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
}


static LRESULT
TOOLBAR_Customize (WND *wndPtr)
{
    FIXME (toolbar, "customization not implemented!\n");

    return 0;
}


static LRESULT
TOOLBAR_DeleteButton (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex = (INT32)wParam;

    if ((nIndex < 0) || (nIndex >= infoPtr->nNumButtons))
	return FALSE;

    if (infoPtr->nNumButtons == 1) {
	TRACE (toolbar, " simple delete!\n");
	HeapFree (GetProcessHeap (), 0, infoPtr->buttons);
	infoPtr->buttons = NULL;
	infoPtr->nNumButtons = 0;
    }
    else {
	TBUTTON_INFO *oldButtons = infoPtr->buttons;
        TRACE(toolbar, "complex delete! [nIndex=%d]\n", nIndex);

	infoPtr->nNumButtons--;
	infoPtr->buttons = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
				      sizeof (TBUTTON_INFO) * infoPtr->nNumButtons);
        if (nIndex > 0) {
            memcpy (&infoPtr->buttons[0], &oldButtons[0],
                    nIndex * sizeof(TBUTTON_INFO));
        }

        if (nIndex < infoPtr->nNumButtons) {
            memcpy (&infoPtr->buttons[nIndex], &oldButtons[nIndex+1],
                    (infoPtr->nNumButtons - nIndex) * sizeof(TBUTTON_INFO));
        }

        HeapFree (GetProcessHeap (), 0, oldButtons);
    }

    TOOLBAR_CalcToolbar (wndPtr);

    InvalidateRect32 (wndPtr->hwndSelf, NULL, TRUE);
    UpdateWindow32 (wndPtr->hwndSelf);

    return TRUE;
}


static LRESULT
TOOLBAR_EnableButton (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    HDC32 hdc;
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    if (LOWORD(lParam) == FALSE)
	btnPtr->fsState &= ~(TBSTATE_ENABLED | TBSTATE_PRESSED);
    else
	btnPtr->fsState |= TBSTATE_ENABLED;

    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


// << TOOLBAR_GetAnchorHighlight >>


static LRESULT
TOOLBAR_GetBitmap (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return 0;

    return infoPtr->buttons[nIndex].iBitmap;
}


static LRESULT
TOOLBAR_GetBitmapFlags (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    FIXME (toolbar, "stub!\n");
    return 0;
}


static LRESULT
TOOLBAR_GetButton (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    LPTBBUTTON lpTbb = (LPTBBUTTON)lParam;
    INT32 nIndex = (INT32)wParam;
    TBUTTON_INFO *btnPtr;

    if (infoPtr == NULL) return FALSE;
    if (lpTbb == NULL) return FALSE;

    if ((nIndex < 0) || (nIndex >= infoPtr->nNumButtons))
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    lpTbb->iBitmap   = btnPtr->iBitmap;
    lpTbb->idCommand = btnPtr->idCommand;
    lpTbb->fsState   = btnPtr->fsState;
    lpTbb->fsStyle   = btnPtr->fsStyle;
    lpTbb->dwData    = btnPtr->dwData;
    lpTbb->iString   = btnPtr->iString;

    return TRUE;
}


// << TOOLBAR_GetButtonInfo >>


static LRESULT
TOOLBAR_GetButtonSize (WND *wndPtr)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    return MAKELONG((WORD)infoPtr->nButtonWidth,
		    (WORD)infoPtr->nButtonHeight);
}


static LRESULT
TOOLBAR_GetButtonText32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex, nStringIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return -1;

    nStringIndex = infoPtr->buttons[nIndex].iString;

    TRACE (toolbar, "index=%d stringIndex=%d\n", nIndex, nStringIndex);

    if ((nStringIndex < 0) || (nStringIndex >= infoPtr->nNumStrings))
	return -1;

    if (lParam == 0) return -1;

    lstrcpy32A ((LPSTR)lParam, (LPSTR)infoPtr->strings[nStringIndex]);

    return lstrlen32A ((LPSTR)infoPtr->strings[nStringIndex]);
}


// << TOOLBAR_GetButtonText32W >>
// << TOOLBAR_GetColorScheme >>
// << TOOLBAR_GetDisabledImageList >>
// << TOOLBAR_GetExtendedStyle >>
// << TOOLBAR_GetHotImageList >>
// << TOOLBAR_GetHotItem >>
// << TOOLBAR_GetImageList >>
// << TOOLBAR_GetInsertMark >>
// << TOOLBAR_GetInsertMarkColor >>


static LRESULT
TOOLBAR_GetItemRect (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    LPRECT32     lpRect;
    INT32        nIndex;

    if (infoPtr == NULL) return FALSE;
    nIndex = (INT32)wParam;
    btnPtr = &infoPtr->buttons[nIndex];
    if ((nIndex < 0) || (nIndex >= infoPtr->nNumButtons))
	return FALSE;
    lpRect = (LPRECT32)lParam;
    if (lpRect == NULL) return FALSE;
    if (btnPtr->fsState & TBSTATE_HIDDEN) return FALSE;
    
    lpRect->left   = btnPtr->rect.left;
    lpRect->right  = btnPtr->rect.right;
    lpRect->bottom = btnPtr->rect.bottom;
    lpRect->top    = btnPtr->rect.top;

    return TRUE;
}


// << TOOLBAR_GetMaxSize >>
// << TOOLBAR_GetObject >>
// << TOOLBAR_GetPadding >>
// << TOOLBAR_GetRect >>


static LRESULT
TOOLBAR_GetRows (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    if (wndPtr->dwStyle & TBSTYLE_WRAPABLE)
	return infoPtr->nMaxRows;
    else
	return 1;
}


static LRESULT
TOOLBAR_GetState (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1) return -1;

    return infoPtr->buttons[nIndex].fsState;
}


static LRESULT
TOOLBAR_GetStyle (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1) return -1;

    return infoPtr->buttons[nIndex].fsStyle;
}


// << TOOLBAR_GetTextRows >>


static LRESULT
TOOLBAR_GetToolTips (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    if (infoPtr == NULL) return 0;
    return infoPtr->hwndToolTip;
}


// << TOOLBAR_GetUnicodeFormat >>


static LRESULT
TOOLBAR_HideButton (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    if (LOWORD(lParam) == FALSE)
	btnPtr->fsState &= ~TBSTATE_HIDDEN;
    else
	btnPtr->fsState |= TBSTATE_HIDDEN;

    TOOLBAR_CalcToolbar (wndPtr);

    InvalidateRect32 (wndPtr->hwndSelf, NULL, TRUE);
    UpdateWindow32 (wndPtr->hwndSelf);

    return TRUE;
}


static LRESULT
TOOLBAR_HitTest (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    return TOOLBAR_InternalHitTest (wndPtr, (LPPOINT32)lParam);
}


static LRESULT
TOOLBAR_Indeterminate (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    HDC32 hdc;
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    if (LOWORD(lParam) == FALSE)
	btnPtr->fsState &= ~TBSTATE_INDETERMINATE;
    else
	btnPtr->fsState |= TBSTATE_INDETERMINATE;

    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


static LRESULT
TOOLBAR_InsertButton32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    LPTBBUTTON lpTbb = (LPTBBUTTON)lParam;
    INT32 nIndex = (INT32)wParam;
    TBUTTON_INFO *oldButtons;
    HDC32 hdc;

    if (lpTbb == NULL) return FALSE;
    if (nIndex < 0) return FALSE;

    TRACE (toolbar, "inserting button index=%d\n", nIndex);
    if (nIndex > infoPtr->nNumButtons) {
	nIndex = infoPtr->nNumButtons;
	TRACE (toolbar, "adjust index=%d\n", nIndex);
    }

    oldButtons = infoPtr->buttons;
    infoPtr->nNumButtons++;
    infoPtr->buttons = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
				  sizeof (TBUTTON_INFO) * infoPtr->nNumButtons);
    /* pre insert copy */
    if (nIndex > 0) {
	memcpy (&infoPtr->buttons[0], &oldButtons[0],
		nIndex * sizeof(TBUTTON_INFO));
    }

    /* insert new button */
    infoPtr->buttons[nIndex].iBitmap   = lpTbb->iBitmap;
    infoPtr->buttons[nIndex].idCommand = lpTbb->idCommand;
    infoPtr->buttons[nIndex].fsState   = lpTbb->fsState;
    infoPtr->buttons[nIndex].fsStyle   = lpTbb->fsStyle;
    infoPtr->buttons[nIndex].dwData    = lpTbb->dwData;
    infoPtr->buttons[nIndex].iString   = lpTbb->iString;

    /* post insert copy */
    if (nIndex < infoPtr->nNumButtons - 1) {
	memcpy (&infoPtr->buttons[nIndex+1], &oldButtons[nIndex],
		(infoPtr->nNumButtons - nIndex - 1) * sizeof(TBUTTON_INFO));
    }

    HeapFree (GetProcessHeap (), 0, oldButtons);

    TOOLBAR_CalcToolbar (wndPtr);

    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


// << TOOLBAR_InsertButton32W >>
// << TOOLBAR_InsertMarkHitTest >>


static LRESULT
TOOLBAR_IsButtonChecked (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_CHECKED);
}


static LRESULT
TOOLBAR_IsButtonEnabled (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_ENABLED);
}


static LRESULT
TOOLBAR_IsButtonHidden (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_HIDDEN);
}


static LRESULT
TOOLBAR_IsButtonHighlighted (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_MARKED);
}


static LRESULT
TOOLBAR_IsButtonIndeterminate (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_INDETERMINATE);
}


static LRESULT
TOOLBAR_IsButtonPressed (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_PRESSED);
}


// << TOOLBAR_LoadImages >>
// << TOOLBAR_MapAccelerator >>
// << TOOLBAR_MarkButton >>
// << TOOLBAR_MoveButton >>


static LRESULT
TOOLBAR_PressButton (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    HDC32 hdc;
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    if (LOWORD(lParam) == FALSE)
	btnPtr->fsState &= ~TBSTATE_PRESSED;
    else
	btnPtr->fsState |= TBSTATE_PRESSED;

    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


// << TOOLBAR_ReplaceBitmap >>


static LRESULT
TOOLBAR_SaveRestore32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    LPTBSAVEPARAMS32A lpSave = (LPTBSAVEPARAMS32A)lParam;

    if (lpSave == NULL) return 0;

    if ((BOOL32)wParam) {
	/* save toolbar information */
	FIXME (toolbar, "save to \"%s\" \"%s\"\n",
	       lpSave->pszSubKey, lpSave->pszValueName);


    }
    else {
	/* restore toolbar information */

	FIXME (toolbar, "restore from \"%s\" \"%s\"\n",
	       lpSave->pszSubKey, lpSave->pszValueName);


    }

    return 0;
}


// << TOOLBAR_SaveRestore32W >>
// << TOOLBAR_SetAnchorHighlight >>


static LRESULT
TOOLBAR_SetBitmapSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    if ((LOWORD(lParam) <= 0) || (HIWORD(lParam)<=0))
	return FALSE;

    infoPtr->nBitmapWidth = (INT32)LOWORD(lParam);
    infoPtr->nBitmapHeight = (INT32)HIWORD(lParam);

    return TRUE;
}


// << TOOLBAR_SetButtonInfo >>


static LRESULT
TOOLBAR_SetButtonSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    if ((LOWORD(lParam) <= 0) || (HIWORD(lParam)<=0))
	return FALSE;

    infoPtr->nButtonWidth = (INT32)LOWORD(lParam);
    infoPtr->nButtonHeight = (INT32)HIWORD(lParam);

    return TRUE;
}


// << TOOLBAR_SetButtonWidth >>


static LRESULT
TOOLBAR_SetCmdId (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex = (INT32)wParam;

    if ((nIndex < 0) || (nIndex >= infoPtr->nNumButtons))
	return FALSE;

    infoPtr->buttons[nIndex].idCommand = (INT32)lParam;

    return TRUE;
}


// << TOOLBAR_SetColorScheme >>
// << TOOLBAR_SetDisabledImageList >>
// << TOOLBAR_SetDrawTextFlags >>
// << TOOLBAR_SetExtendedStyle >>
// << TOOLBAR_SetHotImageList >>
// << TOOLBAR_SetHotItem >>
// << TOOLBAR_SetImageList >>


static LRESULT
TOOLBAR_SetIndent (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    HDC32 hdc;

    infoPtr->nIndent = (INT32)wParam;
    TOOLBAR_CalcToolbar (wndPtr);
    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


// << TOOLBAR_SetInsertMark >>
// << TOOLBAR_SetInsertMarkColor >>
// << TOOLBAR_SetMaxTextRows >>
// << TOOLBAR_SetPadding >>


static LRESULT
TOOLBAR_SetParent (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    HWND32 hwndOldNotify;

    if (infoPtr == NULL) return 0;
    hwndOldNotify = infoPtr->hwndNotify;
    infoPtr->hwndNotify = (HWND32)wParam;

    return hwndOldNotify;
}


static LRESULT
TOOLBAR_SetRows (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    FIXME (toolbar, "support multiple rows!\n");

    return 0;
}


static LRESULT
TOOLBAR_SetState (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    HDC32 hdc;
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    btnPtr->fsState = LOWORD(lParam);

    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


static LRESULT
TOOLBAR_SetStyle (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    HDC32 hdc;
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    btnPtr->fsStyle = LOWORD(lParam);

    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


static LRESULT
TOOLBAR_SetToolTips (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    if (infoPtr == NULL) return 0;
    infoPtr->hwndToolTip = (HWND32)wParam;
    return 0;
}


// << TOOLBAR_SetUnicodeFormat >>


static LRESULT
TOOLBAR_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    /* initialize info structure */
    infoPtr->nButtonHeight = 22;
    infoPtr->nButtonWidth = 23;
    infoPtr->nBitmapHeight = 15;
    infoPtr->nBitmapWidth = 16;

    infoPtr->nHeight = infoPtr->nButtonHeight + TOP_BORDER + BOTTOM_BORDER;
    infoPtr->nMaxRows = 1;

    infoPtr->bCaptured = 0;
    infoPtr->nButtonDown = -1;
    infoPtr->nOldHit = -1;

    infoPtr->hwndNotify = GetParent32 (wndPtr->hwndSelf);
    infoPtr->bTransparent = (wndPtr->dwStyle & TBSTYLE_FLAT);
    infoPtr->nHotItem = -1;

    return 0;
}


static LRESULT
TOOLBAR_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    /* delete tool tip */
    if (infoPtr->hwndToolTip)
	DestroyWindow32 (infoPtr->hwndToolTip);

    /* delete button data */
    if (infoPtr->buttons)
	HeapFree (GetProcessHeap (), 0, infoPtr->buttons);

    /* delete strings */
    if (infoPtr->strings) {
	INT32 i;
	for (i = 0; i < infoPtr->nNumStrings; i++)
	    if (infoPtr->strings[i])
		HeapFree (GetProcessHeap (), 0, infoPtr->strings[i]);

	HeapFree (GetProcessHeap (), 0, infoPtr->strings);
    }

    /* destroy default image list */
    if (infoPtr->himlDef)
	ImageList_Destroy (infoPtr->himlDef);

    /* destroy disabled image list */
    if (infoPtr->himlDis)
	ImageList_Destroy (infoPtr->himlDis);

    /* destroy hot image list */
    if (infoPtr->himlHot)
	ImageList_Destroy (infoPtr->himlHot);

    /* free toolbar info data */
    HeapFree (GetProcessHeap (), 0, infoPtr);

    return 0;
}


static LRESULT
TOOLBAR_LButtonDblClk (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    POINT32 pt;
    INT32   nHit;
    HDC32   hdc;

    pt.x = (INT32)LOWORD(lParam);
    pt.y = (INT32)HIWORD(lParam);
    nHit = TOOLBAR_InternalHitTest (wndPtr, &pt);

    if (nHit >= 0) {
	btnPtr = &infoPtr->buttons[nHit];
	if (!(btnPtr->fsState & TBSTATE_ENABLED))
	    return 0;
	SetCapture32 (wndPtr->hwndSelf);
	infoPtr->bCaptured = TRUE;
	infoPtr->nButtonDown = nHit;

	btnPtr->fsState |= TBSTATE_PRESSED;

	hdc = GetDC32 (wndPtr->hwndSelf);
	TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
	ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }
    else if (wndPtr->dwStyle & CCS_ADJUSTABLE)
	TOOLBAR_Customize (wndPtr);

    return 0;
}


static LRESULT
TOOLBAR_LButtonDown (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    POINT32 pt;
    INT32   nHit;
    HDC32   hdc;

    pt.x = (INT32)LOWORD(lParam);
    pt.y = (INT32)HIWORD(lParam);
    nHit = TOOLBAR_InternalHitTest (wndPtr, &pt);

    if (nHit >= 0) {
	btnPtr = &infoPtr->buttons[nHit];
	if (!(btnPtr->fsState & TBSTATE_ENABLED))
	    return 0;

	SetCapture32 (wndPtr->hwndSelf);
	infoPtr->bCaptured = TRUE;
	infoPtr->nButtonDown = nHit;
	infoPtr->nOldHit = nHit;

	btnPtr->fsState |= TBSTATE_PRESSED;

	hdc = GetDC32 (wndPtr->hwndSelf);
	TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
	ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }
    

    return 0;
}


static LRESULT
TOOLBAR_LButtonUp (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    POINT32 pt;
    INT32   nHit;
    INT32   nOldIndex = -1;
    HDC32   hdc;
    BOOL32  bSendMessage = TRUE;

    pt.x = (INT32)LOWORD(lParam);
    pt.y = (INT32)HIWORD(lParam);
    nHit = TOOLBAR_InternalHitTest (wndPtr, &pt);

    if ((infoPtr->bCaptured) && (infoPtr->nButtonDown >= 0)) {
	infoPtr->bCaptured = FALSE;
	ReleaseCapture ();
	btnPtr = &infoPtr->buttons[infoPtr->nButtonDown];
	btnPtr->fsState &= ~TBSTATE_PRESSED;

	if (nHit == infoPtr->nButtonDown) {
	    if (btnPtr->fsStyle & TBSTYLE_CHECK) {
		if (btnPtr->fsStyle & TBSTYLE_GROUP) {
		    nOldIndex = TOOLBAR_GetCheckedGroupButtonIndex (infoPtr,
			infoPtr->nButtonDown);
		    if (nOldIndex == infoPtr->nButtonDown)
			bSendMessage = FALSE;
		    if ((nOldIndex != infoPtr->nButtonDown) && 
			(nOldIndex != -1))
			infoPtr->buttons[nOldIndex].fsState &= ~TBSTATE_CHECKED;
		    btnPtr->fsState |= TBSTATE_CHECKED;
		}
		else {
		    if (btnPtr->fsState & TBSTATE_CHECKED)
			btnPtr->fsState &= ~TBSTATE_CHECKED;
		    else
			btnPtr->fsState |= TBSTATE_CHECKED;
		}
	    }
	}
	else
	    bSendMessage = FALSE;

	hdc = GetDC32 (wndPtr->hwndSelf);
	if (nOldIndex != -1)
	    TOOLBAR_DrawButton (wndPtr, &infoPtr->buttons[nOldIndex], hdc);
	TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
	ReleaseDC32 (wndPtr->hwndSelf, hdc);

	if (bSendMessage)
	    SendMessage32A (infoPtr->hwndNotify, WM_COMMAND,
			    MAKEWPARAM(btnPtr->idCommand, 0),
			    (LPARAM)wndPtr->hwndSelf);

	infoPtr->nButtonDown = -1;
	infoPtr->nOldHit = -1;
    }

    return 0;
}


static LRESULT
TOOLBAR_MouseMove (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    POINT32 pt;
    INT32   nHit;
    HDC32   hdc;

    pt.x = (INT32)LOWORD(lParam);
    pt.y = (INT32)HIWORD(lParam);
    nHit = TOOLBAR_InternalHitTest (wndPtr, &pt);

    if (infoPtr->bCaptured) {
	if (infoPtr->nOldHit != nHit) {
	    btnPtr = &infoPtr->buttons[infoPtr->nButtonDown];
	    if (infoPtr->nOldHit == infoPtr->nButtonDown) {
		btnPtr->fsState &= ~TBSTATE_PRESSED;
		hdc = GetDC32 (wndPtr->hwndSelf);
		TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
		ReleaseDC32 (wndPtr->hwndSelf, hdc);
	    }
	    else if (nHit == infoPtr->nButtonDown) {
		btnPtr->fsState |= TBSTATE_PRESSED;
		hdc = GetDC32 (wndPtr->hwndSelf);
		TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
		ReleaseDC32 (wndPtr->hwndSelf, hdc);
	    }
	}
	infoPtr->nOldHit = nHit;
    }

    return 0;
}


static LRESULT
TOOLBAR_NCCalcSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    if (!(wndPtr->dwStyle & CCS_NODIVIDER)) {
	LPRECT32 winRect  = (LPRECT32)lParam;
	winRect->top    += 2;   
	winRect->bottom += 2;   
    }

    return DefWindowProc32A (wndPtr->hwndSelf, WM_NCCALCSIZE, wParam, lParam);
}


static LRESULT
TOOLBAR_NCCreate (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (TOOLBAR_INFO *)HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY,
                                   sizeof(TOOLBAR_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;

    if (infoPtr == NULL) {
	ERR (toolbar, "could not allocate info memory!\n");
	return 0;
    }

    if ((TOOLBAR_INFO*)wndPtr->wExtra[0] != infoPtr) {
	ERR (toolbar, "pointer assignment error!\n");
	return 0;
    }

    /* this is just for security (reliable??)*/
    infoPtr->dwStructSize = sizeof(TBBUTTON);

    return DefWindowProc32A (wndPtr->hwndSelf, WM_NCCREATE, wParam, lParam);
}


static LRESULT
TOOLBAR_NCPaint (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    HDC32 hdc;
    RECT32 rect;
    HWND32 hwnd = wndPtr->hwndSelf;

    if ( wndPtr->dwStyle & WS_MINIMIZE ||
	!WIN_IsWindowDrawable( wndPtr, 0 )) return 0; /* Nothing to do */

    DefWindowProc32A (hwnd, WM_NCPAINT, wParam, lParam);

    if (!(hdc = GetDCEx32( hwnd, 0, DCX_USESTYLE | DCX_WINDOW ))) return 0;

    if (ExcludeVisRect( hdc, wndPtr->rectClient.left-wndPtr->rectWindow.left,
		        wndPtr->rectClient.top-wndPtr->rectWindow.top,
		        wndPtr->rectClient.right-wndPtr->rectWindow.left,
		        wndPtr->rectClient.bottom-wndPtr->rectWindow.top )
	== NULLREGION)
    {
	ReleaseDC32( hwnd, hdc );
	return 0;
    }

    if (!(wndPtr->flags & WIN_MANAGED)) {
	if (!(wndPtr->dwStyle & CCS_NODIVIDER)) {
	    rect.left = wndPtr->rectClient.left;
	    rect.top = wndPtr->rectClient.top - 2;
	    rect.right = wndPtr->rectClient.right;

	    SelectObject32 ( hdc, GetSysColorPen32 (COLOR_3DSHADOW));
	    MoveToEx32 (hdc, rect.left, rect.top, NULL);
	    LineTo32 (hdc, rect.right, rect.top);
	    rect.top++;
	    SelectObject32 ( hdc, GetSysColorPen32 (COLOR_3DHILIGHT));
	    MoveToEx32 (hdc, rect.left, rect.top, NULL);
	    LineTo32 (hdc, rect.right, rect.top);
	}

    }

    ReleaseDC32( hwnd, hdc );

    return 0;
}


static LRESULT
TOOLBAR_Paint (WND *wndPtr, WPARAM32 wParam)
{
    HDC32 hdc;
    PAINTSTRUCT32 ps;

    hdc = wParam==0 ? BeginPaint32 (wndPtr->hwndSelf, &ps) : (HDC32)wParam;
    TOOLBAR_Refresh (wndPtr, hdc);
    if (!wParam)
	EndPaint32 (wndPtr->hwndSelf, &ps);
    return 0;
}


static LRESULT
TOOLBAR_Size (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    RECT32 parent_rect;
    HWND32 parent;
    INT32  x, y, cx, cy;
    INT32  flags;
    UINT32 uPosFlags = 0;

    flags = (INT32) wParam;

    /* FIXME for flags =
     * SIZE_MAXIMIZED, SIZE_MAXSHOW, SIZE_MINIMIZED
     */

    if (flags == SIZE_RESTORED) {
	/* width and height don't apply */
	parent = GetParent32 (wndPtr->hwndSelf);
	GetClientRect32(parent, &parent_rect);

	if (wndPtr->dwStyle & CCS_NORESIZE)
	    uPosFlags |= SWP_NOSIZE;
	else {
	    infoPtr->nWidth = parent_rect.right - parent_rect.left;
	    TOOLBAR_CalcToolbar (wndPtr);
	    cy = infoPtr->nHeight;
	    cx = infoPtr->nWidth;
	}

	if (wndPtr->dwStyle & CCS_NOPARENTALIGN) {
	    uPosFlags |= SWP_NOMOVE;
	}

	if (!(wndPtr->dwStyle & CCS_NODIVIDER))
	    cy += 2;

	SetWindowPos32 (wndPtr->hwndSelf, 0, parent_rect.left, parent_rect.top,
			cx, cy, uPosFlags | SWP_NOZORDER);
    }
    return 0;
}


LRESULT WINAPI
ToolbarWindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {
	case TB_ADDBITMAP:
	    return TOOLBAR_AddBitmap (wndPtr, wParam, lParam);

	case TB_ADDBUTTONS32A:
	    return TOOLBAR_AddButtons32A (wndPtr, wParam, lParam);

//	case TB_ADDBUTTONS32W:

	case TB_ADDSTRING32A:
	    return TOOLBAR_AddString32A (wndPtr, wParam, lParam);

//	case TB_ADDSTRING32W:

	case TB_AUTOSIZE:
	    return TOOLBAR_AutoSize (wndPtr, wParam, lParam);

	case TB_BUTTONCOUNT:
	    return TOOLBAR_ButtonCount (wndPtr, wParam, lParam);

	case TB_BUTTONSTRUCTSIZE:
	    return TOOLBAR_ButtonStructSize (wndPtr, wParam, lParam);

	case TB_CHANGEBITMAP:
	    return TOOLBAR_ChangeBitmap (wndPtr, wParam, lParam);

	case TB_CHECKBUTTON:
	    return TOOLBAR_CheckButton (wndPtr, wParam, lParam);

	case TB_COMMANDTOINDEX:
	    return TOOLBAR_CommandToIndex (wndPtr, wParam, lParam);

	case TB_CUSTOMIZE:
	    return TOOLBAR_Customize (wndPtr);

	case TB_DELETEBUTTON:
	    return TOOLBAR_DeleteButton (wndPtr, wParam, lParam);

	case TB_ENABLEBUTTON:
	    return TOOLBAR_EnableButton (wndPtr, wParam, lParam);

//	case TB_GETANCHORHIGHLIGHT:		/* 4.71 */

	case TB_GETBITMAP:
	    return TOOLBAR_GetBitmap (wndPtr, wParam, lParam);

	case TB_GETBITMAPFLAGS:
	    return TOOLBAR_GetBitmapFlags (wndPtr, wParam, lParam);

	case TB_GETBUTTON:
	    return TOOLBAR_GetButton (wndPtr, wParam, lParam);

//	case TB_GETBUTTONINFO:			/* 4.71 */

	case TB_GETBUTTONSIZE:
	    return TOOLBAR_GetButtonSize (wndPtr);

	case TB_GETBUTTONTEXT32A:
	    return TOOLBAR_GetButtonText32A (wndPtr, wParam, lParam);

//	case TB_GETBUTTONTEXT32W:
//	case TB_GETCOLORSCHEME:			/* 4.71 */
//	case TB_GETDISABLEDIMAGELIST:		/* 4.70 */
//	case TB_GETEXTENDEDSTYLE:		/* 4.71 */
//	case TB_GETHOTIMAGELIST:		/* 4.70 */
//	case TB_GETHOTITEM:			/* 4.71 */
//	case TB_GETIMAGELIST:			/* 4.70 */
//	case TB_GETINSERTMARK:			/* 4.71 */
//	case TB_GETINSERTMARKCOLOR:		/* 4.71 */

	case TB_GETITEMRECT:
	    return TOOLBAR_GetItemRect (wndPtr, wParam, lParam);

//	case TB_GETMAXSIZE:			/* 4.71 */
//	case TB_GETOBJECT:			/* 4.71 */
//	case TB_GETPADDING:			/* 4.71 */
//	case TB_GETRECT:			/* 4.70 */

	case TB_GETROWS:
	    return TOOLBAR_GetRows (wndPtr, wParam, lParam);

	case TB_GETSTATE:
	    return TOOLBAR_GetState (wndPtr, wParam, lParam);

	case TB_GETSTYLE:
	    return TOOLBAR_GetStyle (wndPtr, wParam, lParam);

//	case TB_GETTEXTROWS:			/* 4.70 */

	case TB_GETTOOLTIPS:
	    return TOOLBAR_GetToolTips (wndPtr, wParam, lParam);

//	case TB_GETUNICODEFORMAT:

	case TB_HIDEBUTTON:
	    return TOOLBAR_HideButton (wndPtr, wParam, lParam);

	case TB_HITTEST:
	    return TOOLBAR_HitTest (wndPtr, wParam, lParam);

	case TB_INDETERMINATE:
	    return TOOLBAR_Indeterminate (wndPtr, wParam, lParam);

	case TB_INSERTBUTTON32A:
	    return TOOLBAR_InsertButton32A (wndPtr, wParam, lParam);

//	case TB_INSERTBUTTON32W:
//	case TB_INSERTMARKHITTEST:		/* 4.71 */

	case TB_ISBUTTONCHECKED:
	    return TOOLBAR_IsButtonChecked (wndPtr, wParam, lParam);

	case TB_ISBUTTONENABLED:
	    return TOOLBAR_IsButtonEnabled (wndPtr, wParam, lParam);

	case TB_ISBUTTONHIDDEN:
	    return TOOLBAR_IsButtonHidden (wndPtr, wParam, lParam);

	case TB_ISBUTTONHIGHLIGHTED:
	    return TOOLBAR_IsButtonHighlighted (wndPtr, wParam, lParam);

	case TB_ISBUTTONINDETERMINATE:
	    return TOOLBAR_IsButtonIndeterminate (wndPtr, wParam, lParam);

	case TB_ISBUTTONPRESSED:
	    return TOOLBAR_IsButtonPressed (wndPtr, wParam, lParam);

//	case TB_LOADIMAGES:			/* 4.70 */
//	case TB_MAPACCELERATOR32A:		/* 4.71 */
//	case TB_MAPACCELERATOR32W:		/* 4.71 */
//	case TB_MARKBUTTON:			/* 4.71 */
//	case TB_MOVEBUTTON:			/* 4.71 */

	case TB_PRESSBUTTON:
	    return TOOLBAR_PressButton (wndPtr, wParam, lParam);

//	case TB_REPLACEBITMAP:

	case TB_SAVERESTORE32A:
	    return TOOLBAR_SaveRestore32A (wndPtr, wParam, lParam);

//	case TB_SAVERESTORE32W:
//	case TB_SETANCHORHIGHLIGHT:		/* 4.71 */

	case TB_SETBITMAPSIZE:
	    return TOOLBAR_SetBitmapSize (wndPtr, wParam, lParam);

//	case TB_SETBUTTONINFO:			/* 4.71 */

	case TB_SETBUTTONSIZE:
	    return TOOLBAR_SetButtonSize (wndPtr, wParam, lParam);

//	case TB_SETBUTTONWIDTH:			/* 4.70 */

	case TB_SETCMDID:
	    return TOOLBAR_SetCmdId (wndPtr, wParam, lParam);

//	case TB_SETCOLORSCHEME:			/* 4.71 */
//	case TB_SETDISABLEDIMAGELIST:		/* 4.70 */
//	case TB_SETDRAWTEXTFLAGS:		/* 4.71 */
//	case TB_SETEXTENDEDSTYLE:		/* 4.71 */
//	case TB_SETHOTIMAGELIST:		/* 4.70 */
//	case TB_SETHOTITEM:			/* 4.71 */
//	case TB_SETIMAGELIST:			/* 4.70 */

	case TB_SETINDENT:
	    return TOOLBAR_SetIndent (wndPtr, wParam, lParam);

//	case TB_SETINSERTMARK:			/* 4.71 */
//	case TB_SETINSERTMARKCOLOR:		/* 4.71 */
//	case TB_SETMAXTEXTROWS:			/* 4.70 */
//	case TB_SETPADDING:			/* 4.71 */

	case TB_SETPARENT:
	    return TOOLBAR_SetParent (wndPtr, wParam, lParam);

	case TB_SETROWS:
	    return TOOLBAR_SetRows (wndPtr, wParam, lParam);

	case TB_SETSTATE:
	    return TOOLBAR_SetState (wndPtr, wParam, lParam);

	case TB_SETSTYLE:
	    return TOOLBAR_SetStyle (wndPtr, wParam, lParam);

	case TB_SETTOOLTIPS:
	    return TOOLBAR_SetToolTips (wndPtr, wParam, lParam);

//	case TB_SETUNICODEFORMAT:

	case WM_CREATE:
	    return TOOLBAR_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return TOOLBAR_Destroy (wndPtr, wParam, lParam);

//	case WM_ERASEBKGND:
//	    return TOOLBAR_EraseBackground (wndPtr, wParam, lParam);

	case WM_LBUTTONDBLCLK:
	    return TOOLBAR_LButtonDblClk (wndPtr, wParam, lParam);

	case WM_LBUTTONDOWN:
	    return TOOLBAR_LButtonDown (wndPtr, wParam, lParam);

	case WM_LBUTTONUP:
	    return TOOLBAR_LButtonUp (wndPtr, wParam, lParam);

	case WM_MOUSEMOVE:
	    return TOOLBAR_MouseMove (wndPtr, wParam, lParam);

//	case WM_NCACTIVATE:
//	    return TOOLBAR_NCActivate (wndPtr, wParam, lParam);

	case WM_NCCALCSIZE:
	    return TOOLBAR_NCCalcSize (wndPtr, wParam, lParam);

	case WM_NCCREATE:
	    return TOOLBAR_NCCreate (wndPtr, wParam, lParam);

	case WM_NCPAINT:
	    return TOOLBAR_NCPaint (wndPtr, wParam, lParam);

//	case WM_NOTIFY:

	case WM_PAINT:
	    return TOOLBAR_Paint (wndPtr, wParam);

	case WM_SIZE:
	    return TOOLBAR_Size (wndPtr, wParam, lParam);

//	case WM_SYSCOLORCHANGE:

//	case WM_WININICHANGE:

	case WM_CHARTOITEM:
	case WM_COMMAND:
	case WM_DRAWITEM:
	case WM_MEASUREITEM:
	case WM_VKEYTOITEM:
	    return SendMessage32A (GetParent32 (hwnd), uMsg, wParam, lParam);

	default:
	    if (uMsg >= WM_USER)
		ERR (toolbar, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


void
TOOLBAR_Register (void)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (TOOLBARCLASSNAME32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC32)ToolbarWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TOOLBAR_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = (HBRUSH32)(COLOR_3DFACE + 1);
    wndClass.lpszClassName = TOOLBARCLASSNAME32A;
 
    RegisterClass32A (&wndClass);
}


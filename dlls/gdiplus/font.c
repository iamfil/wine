/*
 * Copyright (C) 2007 Google (Evan Stade)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winreg.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL (gdiplus);

#include "objbase.h"

#include "gdiplus.h"
#include "gdiplus_private.h"

static const REAL mm_per_pixel = 25.4;

static inline REAL get_dpi (void)
{
    REAL dpi;
    GpGraphics *graphics;
    HDC hdc = GetDC(0);
    GdipCreateFromHDC (hdc, &graphics);
    GdipGetDpiX(graphics, &dpi);
    ReleaseDC (0, hdc);

    return dpi;
}

static inline REAL point_to_pixel (REAL point)
{
    return point * 1.5;
}

static inline REAL inch_to_pixel (REAL inch)
{
    return inch * get_dpi();
}

static inline REAL document_to_pixel (REAL doc)
{
    return doc * (get_dpi() / 300.0); /* Per MSDN */
}

static inline REAL mm_to_pixel (REAL mm)
{
    return mm * (get_dpi() / mm_per_pixel);
}

/*******************************************************************************
 * GdipCreateFont [GDIPLUS.@]
 *
 * Create a new font based off of a FontFamily
 *
 * PARAMS
 *  *fontFamily     [I] Family to base the font off of
 *  emSize          [I] Size of the font
 *  style           [I] Bitwise OR of FontStyle enumeration
 *  unit            [I] Unit emSize is measured in
 *  **font          [I] the resulting Font object
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: InvalidParameter if fontfamily or font is NULL.
 *  FAILURE: FontFamilyNotFound if an invalid FontFamily is given
 *
 * NOTES
 *  UnitDisplay is unsupported.
 *  emSize is stored separately from lfHeight, to hold the fraction.
 */
GpStatus WINGDIPAPI GdipCreateFont(GDIPCONST GpFontFamily *fontFamily,
                        REAL emSize, INT style, Unit unit, GpFont **font)
{
    WCHAR facename[LF_FACESIZE];
    LOGFONTW* lfw;
    TEXTMETRICW* tmw;
    GpStatus stat;

    if ((!fontFamily && fontFamily->FamilyName && font))
        return InvalidParameter;

    TRACE("%p (%s), %f, %d, %d, %p\n", fontFamily,
            debugstr_w(fontFamily->FamilyName), emSize, style, unit, font);

    stat = GdipGetFamilyName (fontFamily, facename, 0);
    if (stat != Ok) return stat;
    *font = GdipAlloc(sizeof(GpFont));

    tmw = fontFamily->tmw;
    lfw = &((*font)->lfw);
    ZeroMemory(&(*lfw), sizeof(*lfw));

    lfw->lfWeight = tmw->tmWeight;
    lfw->lfItalic = tmw->tmItalic;
    lfw->lfUnderline = tmw->tmUnderlined;
    lfw->lfStrikeOut = tmw->tmStruckOut;
    lfw->lfCharSet = tmw->tmCharSet;
    lfw->lfPitchAndFamily = tmw->tmPitchAndFamily;
    lstrcpynW((lfw->lfFaceName), facename, sizeof(WCHAR) * LF_FACESIZE);

    switch (unit)
    {
        case UnitWorld:
            /* FIXME: Figure out when World != Pixel */
            lfw->lfHeight = emSize; break;
        case UnitDisplay:
            FIXME("Unknown behavior for UnitDisplay! Please report!\n");
            /* FIXME: Figure out how this works...
             * MSDN says that if "DISPLAY" is a monitor, then pixel should be
             * used. That's not what I got. Tests on Windows revealed no output,
             * and the tests in tests/font crash windows */
            lfw->lfHeight = 0; break;
        case UnitPixel:
            lfw->lfHeight = emSize; break;
        case UnitPoint:
            lfw->lfHeight = point_to_pixel(emSize); break;
        case UnitInch:
            lfw->lfHeight = inch_to_pixel(emSize); break;
        case UnitDocument:
            lfw->lfHeight = document_to_pixel(emSize); break;
        case UnitMillimeter:
            lfw->lfHeight = mm_to_pixel(emSize); break;
    }

    lfw->lfHeight *= -1;

    lfw->lfWeight = style & FontStyleBold ? 700 : 400;
    lfw->lfItalic = style & FontStyleItalic;
    lfw->lfUnderline = style & FontStyleUnderline;
    lfw->lfStrikeOut = style & FontStyleStrikeout;

    (*font)->unit = unit;
    (*font)->emSize = emSize;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateFontFromLogfontW(HDC hdc,
    GDIPCONST LOGFONTW *logfont, GpFont **font)
{
    HFONT hfont, oldfont;
    TEXTMETRICW textmet;

    if(!logfont || !font)
        return InvalidParameter;

    *font = GdipAlloc(sizeof(GpFont));
    if(!*font)  return OutOfMemory;

    memcpy(&(*font)->lfw.lfFaceName, logfont->lfFaceName, LF_FACESIZE *
           sizeof(WCHAR));
    (*font)->lfw.lfHeight = logfont->lfHeight;
    (*font)->lfw.lfItalic = logfont->lfItalic;
    (*font)->lfw.lfUnderline = logfont->lfUnderline;
    (*font)->lfw.lfStrikeOut = logfont->lfStrikeOut;

    (*font)->emSize = logfont->lfHeight;
    (*font)->unit = UnitPixel;

    hfont = CreateFontIndirectW(&(*font)->lfw);
    oldfont = SelectObject(hdc, hfont);
    GetTextMetricsW(hdc, &textmet);

    (*font)->lfw.lfHeight = -textmet.tmHeight;
    (*font)->lfw.lfWeight = textmet.tmWeight;

    SelectObject(hdc, oldfont);
    DeleteObject(hfont);

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateFontFromLogfontA(HDC hdc,
    GDIPCONST LOGFONTA *lfa, GpFont **font)
{
    LOGFONTW lfw;

    if(!lfa || !font)
        return InvalidParameter;

    memcpy(&lfw, lfa, FIELD_OFFSET(LOGFONTA,lfFaceName) );

    if(!MultiByteToWideChar(CP_ACP, 0, lfa->lfFaceName, -1, lfw.lfFaceName, LF_FACESIZE))
        return GenericError;

    GdipCreateFontFromLogfontW(hdc, &lfw, font);

    return Ok;
}

GpStatus WINGDIPAPI GdipDeleteFont(GpFont* font)
{
    if(!font)
        return InvalidParameter;

    GdipFree(font);

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateFontFromDC(HDC hdc, GpFont **font)
{
    HFONT hfont;
    LOGFONTW lfw;

    if(!font)
        return InvalidParameter;

    hfont = (HFONT)GetCurrentObject(hdc, OBJ_FONT);
    if(!hfont)
        return GenericError;

    if(!GetObjectW(hfont, sizeof(LOGFONTW), &lfw))
        return GenericError;

    return GdipCreateFontFromLogfontW(hdc, &lfw, font);
}

/******************************************************************************
 * GdipGetFontSize [GDIPLUS.@]
 *
 * Returns the size of the font in Units
 *
 * PARAMS
 *  *font       [I] The font to retrieve size from
 *  *size       [O] Pointer to hold retrieved value
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: InvalidParamter (font or size was NULL)
 *
 * NOTES
 *  Size returned is actually emSize -- not internal size used for drawing.
 */
GpStatus WINGDIPAPI GdipGetFontSize(GpFont *font, REAL *size)
{
    if (!(font && size)) return InvalidParameter;

    *size = font->emSize;

    return Ok;
}

/*******************************************************************************
 * GdipGetFontUnit  [GDIPLUS.@]
 *
 * PARAMS
 *  font    [I] Font to retrieve from
 *  unit    [O] Return value
 *
 * RETURNS
 *  FAILURE: font or unit was NULL
 *  OK: otherwise
 */
GpStatus WINGDIPAPI GdipGetFontUnit(GpFont *font, Unit *unit)
{
    if (!(font && unit)) return InvalidParameter;

    *unit = font->unit;

    return Ok;
}

/* FIXME: use graphics */
GpStatus WINGDIPAPI GdipGetLogFontW(GpFont *font, GpGraphics *graphics,
    LOGFONTW *lfw)
{
    if(!font || !graphics || !lfw)
        return InvalidParameter;

    *lfw = font->lfw;

    return Ok;
}

GpStatus WINGDIPAPI GdipCloneFont(GpFont *font, GpFont **cloneFont)
{
    if(!font || !cloneFont)
        return InvalidParameter;

    *cloneFont = GdipAlloc(sizeof(GpFont));
    if(!*cloneFont)    return OutOfMemory;

    **cloneFont = *font;

    return Ok;
}

/* Borrowed from GDI32 */
static INT CALLBACK is_font_installed_proc(const LOGFONTW *elf,
                            const TEXTMETRICW *ntm, DWORD type, LPARAM lParam)
{
    return 0;
}

static BOOL is_font_installed(const WCHAR *name)
{
    HDC hdc = GetDC(0);
    BOOL ret = FALSE;

    if(!EnumFontFamiliesW(hdc, name, is_font_installed_proc, 0))
        ret = TRUE;

    ReleaseDC(0, hdc);
    return ret;
}

/*******************************************************************************
 * GdipCreateFontFamilyFromName [GDIPLUS.@]
 *
 * Creates a font family object based on a supplied name
 *
 * PARAMS
 *  name               [I] Name of the font
 *  fontCollection     [I] What font collection (if any) the font belongs to (may be NULL)
 *  FontFamily         [O] Pointer to the resulting FontFamily object
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: FamilyNotFound if the requested FontFamily does not exist on the system
 *  FAILURE: Invalid parameter if FontFamily or name is NULL
 *
 * NOTES
 *   If fontCollection is NULL then the object is not part of any collection
 *
 */

GpStatus WINGDIPAPI GdipCreateFontFamilyFromName(GDIPCONST WCHAR *name,
                                        GpFontCollection *fontCollection,
                                        GpFontFamily **FontFamily)
{
    GpFontFamily* ffamily;
    HDC hdc;
    HFONT hFont;
    LOGFONTW lfw;

    TRACE("%s, %p %p\n", debugstr_w(name), fontCollection, FontFamily);

    if (!(name && FontFamily))
        return InvalidParameter;
    if (fontCollection)
        FIXME("No support for FontCollections yet!\n");
    if (!is_font_installed(name))
        return FontFamilyNotFound;

    ffamily = GdipAlloc(sizeof (GpFontFamily));
    if (!ffamily) return OutOfMemory;
    ffamily->tmw = GdipAlloc(sizeof (TEXTMETRICW));
    if (!ffamily->tmw) {GdipFree (ffamily); return OutOfMemory;}

    hdc = GetDC(0);
    lstrcpynW(lfw.lfFaceName, name, sizeof(WCHAR) * LF_FACESIZE);
    hFont = CreateFontIndirectW (&lfw);
    SelectObject(hdc, hFont);

    GetTextMetricsW(hdc, ffamily->tmw);

    ffamily->FamilyName = GdipAlloc(LF_FACESIZE * sizeof (WCHAR));
    if (!ffamily->FamilyName)
    {
        GdipFree(ffamily);
        return OutOfMemory;
    }

    lstrcpynW(ffamily->FamilyName, name, sizeof(WCHAR) * LF_FACESIZE);

    *FontFamily = ffamily;
    ReleaseDC(0, hdc);

    return Ok;
}

/*******************************************************************************
 * GdipGetFamilyName [GDIPLUS.@]
 *
 * Returns the family name into name
 *
 * PARAMS
 *  *family     [I] Family to retrieve from
 *  *name       [O] WCHARS of the family name
 *  LANGID      [I] charset
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: InvalidParameter if family is NULL
 *
 * NOTES
 *   If name is a NULL ptr, then both XP and Vista will crash (so we do as well)
 */
GpStatus WINGDIPAPI GdipGetFamilyName (GDIPCONST GpFontFamily *family,
                                       WCHAR *name, LANGID language)
{
    if (family == NULL)
         return InvalidParameter;

    TRACE("%p, %p, %d\n", family, name, language);

    if (language != LANG_NEUTRAL)
        FIXME("No support for handling of multiple languages!\n");

    lstrcpynW (name, family->FamilyName, LF_FACESIZE);

    return Ok;
}


/*****************************************************************************
 * GdipDeleteFontFamily [GDIPLUS.@]
 *
 * Removes the specified FontFamily
 *
 * PARAMS
 *  *FontFamily         [I] The family to delete
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: InvalidParameter if FontFamily is NULL.
 *
 */
GpStatus WINGDIPAPI GdipDeleteFontFamily(GpFontFamily *FontFamily)
{
    if (!FontFamily)
        return InvalidParameter;
    TRACE("Deleting %p (%s)\n", FontFamily, debugstr_w(FontFamily->FamilyName));

    if (FontFamily->FamilyName) GdipFree (FontFamily->FamilyName);
    if (FontFamily->tmw) GdipFree (FontFamily->tmw);
    GdipFree (FontFamily);

    return Ok;
}

/*****************************************************************************
 * GdipGetGenericFontFamilyMonospace [GDIPLUS.@]
 *
 * Obtains a serif family (Courier New on Windows)
 *
 * PARAMS
 *  **nativeFamily         [I] Where the font will be stored
 *
 * RETURNS
 *  InvalidParameter if nativeFamily is NULL.
 *  Ok otherwise.
 */
GpStatus WINGDIPAPI GdipGetGenericFontFamilyMonospace(GpFontFamily **nativeFamily)
{
    static const WCHAR CourierNew[] = {'C','o','u','r','i','e','r',' ','N','e','w','\0'};

    if (nativeFamily == NULL) return InvalidParameter;

    return GdipCreateFontFamilyFromName(CourierNew, NULL, nativeFamily);
}

/*****************************************************************************
 * GdipGetGenericFontFamilySerif [GDIPLUS.@]
 *
 * Obtains a serif family (Times New Roman on Windows)
 *
 * PARAMS
 *  **nativeFamily         [I] Where the font will be stored
 *
 * RETURNS
 *  InvalidParameter if nativeFamily is NULL.
 *  Ok otherwise.
 */
GpStatus WINGDIPAPI GdipGetGenericFontFamilySerif(GpFontFamily **nativeFamily)
{
    static const WCHAR TimesNewRoman[] = {'T','i','m','e','s',' ','N','e','w',' ','R','o','m','a','n','\0'};

    if (nativeFamily == NULL) return InvalidParameter;

    return GdipCreateFontFamilyFromName(TimesNewRoman, NULL, nativeFamily);
}

/*****************************************************************************
 * GdipGetGenericFontFamilySansSerif [GDIPLUS.@]
 *
 * Obtains a serif family (Microsoft Sans Serif on Windows)
 *
 * PARAMS
 *  **nativeFamily         [I] Where the font will be stored
 *
 * RETURNS
 *  InvalidParameter if nativeFamily is NULL.
 *  Ok otherwise.
 */
GpStatus WINGDIPAPI GdipGetGenericFontFamilySansSerif(GpFontFamily **nativeFamily)
{
    /* FIXME: On Windows this is called Microsoft Sans Serif, this shouldn't
     * affect anything */
    static const WCHAR MSSansSerif[] = {'M','S',' ','S','a','n','s',' ','S','e','r','i','f','\0'};

    if (nativeFamily == NULL) return InvalidParameter;

    return GdipCreateFontFamilyFromName(MSSansSerif, NULL, nativeFamily);
}

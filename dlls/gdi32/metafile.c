/*
 * Metafile functions
 *
 * Copyright  David W. Metcalfe, 1994
 * Copyright  Niels de Carpentier, 1996
 * Copyright  Albrecht Kleine, 1996
 * Copyright  Huw Davies, 1996
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
 *
 * NOTES
 *
 * These functions are primarily involved with metafile playback or anything
 * that touches a HMETAFILE.
 * For recording of metafiles look in graphics/metafiledrv/
 *
 * Note that (32 bit) HMETAFILEs are GDI objects, while HMETAFILE16s are
 * global memory handles so these cannot be interchanged.
 *
 * Memory-based metafiles are just stored as a continuous block of memory with
 * a METAHEADER at the head with METARECORDs appended to it.  mtType is
 * METAFILE_MEMORY (1).  Note this is identical to the disk image of a
 * disk-based metafile - even mtType is METAFILE_MEMORY.
 * 16bit HMETAFILE16s are global handles to this block
 * 32bit HMETAFILEs are GDI handles METAFILEOBJs, which contains a ptr to
 * the memory.
 * Disk-based metafiles are rather different. HMETAFILE16s point to a
 * METAHEADER which has mtType equal to METAFILE_DISK (2).  Following the 9
 * WORDs of the METAHEADER there are a further 3 WORDs of 0, 1 of 0x117, 1
 * more 0, then 2 which may be a time stamp of the file and then the path of
 * the file (METAHEADERDISK). I've copied this for 16bit compatibility.
 *
 * HDMD - 14/4/1999
 */

#include "config.h"

#include <stdarg.h>
#include <string.h>
#include <fcntl.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winternl.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(metafile);

#include "pshpack1.h"
typedef struct
{
    DWORD dw1, dw2, dw3;
    WORD w4;
    CHAR filename[0x100];
} METAHEADERDISK;
#include "poppack.h"

typedef struct
{
    GDIOBJHDR   header;
    METAHEADER  *mh;
} METAFILEOBJ;


/******************************************************************
 *         MF_AddHandle
 *
 *    Add a handle to an external handle table and return the index
 */
static int MF_AddHandle(HANDLETABLE *ht, UINT htlen, HGDIOBJ hobj)
{
    int i;

    for (i = 0; i < htlen; i++)
    {
	if (*(ht->objectHandle + i) == 0)
	{
	    *(ht->objectHandle + i) = hobj;
	    return i;
	}
    }
    return -1;
}


/******************************************************************
 *         MF_Create_HMETATFILE
 *
 * Creates a (32 bit) HMETAFILE object from a METAHEADER
 *
 * HMETAFILEs are GDI objects.
 */
HMETAFILE MF_Create_HMETAFILE(METAHEADER *mh)
{
    HMETAFILE hmf = 0;
    METAFILEOBJ *metaObj = GDI_AllocObject( sizeof(METAFILEOBJ), METAFILE_MAGIC,
					    (HGDIOBJ *)&hmf, NULL );
    if (metaObj)
    {
        metaObj->mh = mh;
        GDI_ReleaseObj( hmf );
    }
    return hmf;
}

/******************************************************************
 *         MF_GetMetaHeader
 *
 * Returns ptr to METAHEADER associated with HMETAFILE
 */
static METAHEADER *MF_GetMetaHeader( HMETAFILE hmf )
{
    METAHEADER *ret = NULL;
    METAFILEOBJ * metaObj = (METAFILEOBJ *)GDI_GetObjPtr( hmf, METAFILE_MAGIC );
    if (metaObj)
    {
        ret = metaObj->mh;
        GDI_ReleaseObj( hmf );
    }
    return ret;
}

/******************************************************************
 *         convert_points
 *
 * Convert an array of POINT16 to an array of POINT.
 * Result must be freed by caller.
 */
static POINT *convert_points( UINT count, POINT16 *pt16 )
{
    UINT i;
    POINT *ret = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*ret) );
    if (ret)
    {
        for (i = 0; i < count; i++)
        {
            ret[i].x = pt16[i].x;
            ret[i].y = pt16[i].y;
        }
    }
    return ret;
}

/******************************************************************
 *          DeleteMetaFile  (GDI32.@)
 *
 *  Delete a memory-based metafile.
 */

BOOL WINAPI DeleteMetaFile( HMETAFILE hmf )
{
    METAFILEOBJ * metaObj = (METAFILEOBJ *)GDI_GetObjPtr( hmf, METAFILE_MAGIC );
    if (!metaObj) return FALSE;
    HeapFree( GetProcessHeap(), 0, metaObj->mh );
    GDI_FreeObject( hmf, metaObj );
    return TRUE;
}

/******************************************************************
 *         MF_ReadMetaFile
 *
 * Returns a pointer to a memory based METAHEADER read in from file HFILE
 *
 */
METAHEADER *MF_ReadMetaFile(HANDLE hfile)
{
    METAHEADER *mh;
    DWORD BytesRead, size;

    size = sizeof(METAHEADER);
    mh = HeapAlloc( GetProcessHeap(), 0, size );
    if(!mh) return NULL;
    if(ReadFile( hfile, mh, size, &BytesRead, NULL) == 0 ||
       BytesRead != size) {
        HeapFree( GetProcessHeap(), 0, mh );
	return NULL;
    }
    if (mh->mtType != METAFILE_MEMORY || mh->mtVersion != MFVERSION ||
        mh->mtHeaderSize != size / 2)
    {
        HeapFree( GetProcessHeap(), 0, mh );
        return NULL;
    }
    size = mh->mtSize * 2;
    mh = HeapReAlloc( GetProcessHeap(), 0, mh, size );
    if(!mh) return NULL;
    size -= sizeof(METAHEADER);
    if(ReadFile( hfile, (char *)mh + sizeof(METAHEADER), size, &BytesRead,
		 NULL) == 0 ||
       BytesRead != size) {
        HeapFree( GetProcessHeap(), 0, mh );
	return NULL;
    }

    if (mh->mtType != METAFILE_MEMORY) {
        WARN("Disk metafile had mtType = %04x\n", mh->mtType);
	mh->mtType = METAFILE_MEMORY;
    }
    return mh;
}

/******************************************************************
 *         GetMetaFileA   (GDI32.@)
 *
 *  Read a metafile from a file. Returns handle to a memory-based metafile.
 */
HMETAFILE WINAPI GetMetaFileA( LPCSTR lpFilename )
{
    METAHEADER *mh;
    HANDLE hFile;

    TRACE("%s\n", lpFilename);

    if(!lpFilename)
        return 0;

    if((hFile = CreateFileA(lpFilename, GENERIC_READ, FILE_SHARE_READ, NULL,
			    OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE)
        return 0;

    mh = MF_ReadMetaFile(hFile);
    CloseHandle(hFile);
    if(!mh) return 0;
    return MF_Create_HMETAFILE( mh );
}

/******************************************************************
 *         GetMetaFileW   (GDI32.@)
 */
HMETAFILE WINAPI GetMetaFileW( LPCWSTR lpFilename )
{
    METAHEADER *mh;
    HANDLE hFile;

    TRACE("%s\n", debugstr_w(lpFilename));

    if(!lpFilename)
        return 0;

    if((hFile = CreateFileW(lpFilename, GENERIC_READ, FILE_SHARE_READ, NULL,
			    OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE)
        return 0;

    mh = MF_ReadMetaFile(hFile);
    CloseHandle(hFile);
    if(!mh) return 0;
    return MF_Create_HMETAFILE( mh );
}


/******************************************************************
 *         MF_LoadDiskBasedMetaFile
 *
 * Creates a new memory-based metafile from a disk-based one.
 */
METAHEADER *MF_LoadDiskBasedMetaFile(METAHEADER *mh)
{
    METAHEADERDISK *mhd;
    HANDLE hfile;
    METAHEADER *mh2;

    if(mh->mtType != METAFILE_DISK) {
        ERR("Not a disk based metafile\n");
	return NULL;
    }
    mhd = (METAHEADERDISK *)((char *)mh + sizeof(METAHEADER));

    if((hfile = CreateFileA(mhd->filename, GENERIC_READ, FILE_SHARE_READ, NULL,
			    OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE) {
        WARN("Can't open file of disk based metafile\n");
        return NULL;
    }
    mh2 = MF_ReadMetaFile(hfile);
    CloseHandle(hfile);
    return mh2;
}

/******************************************************************
 *         MF_CreateMetaHeaderDisk
 *
 * Take a memory based METAHEADER and change it to a disk based METAHEADER
 * associated with filename.  Note: Trashes contents of old one.
 */
METAHEADER *MF_CreateMetaHeaderDisk(METAHEADER *mh, LPCVOID filename, BOOL uni )
{
    METAHEADERDISK *mhd;

    mh = HeapReAlloc( GetProcessHeap(), 0, mh,
		      sizeof(METAHEADER) + sizeof(METAHEADERDISK));
    mh->mtType = METAFILE_DISK;
    mhd = (METAHEADERDISK *)((char *)mh + sizeof(METAHEADER));

    if( uni )
        WideCharToMultiByte(CP_ACP, 0, filename, -1, 
                   mhd->filename, sizeof mhd->filename, NULL, NULL);
    else
        lstrcpynA( mhd->filename, filename, sizeof mhd->filename );
    return mh;
}

/******************************************************************
 *         CopyMetaFileW   (GDI32.@)
 *
 *  Copies the metafile corresponding to hSrcMetaFile to either
 *  a disk file, if a filename is given, or to a new memory based
 *  metafile, if lpFileName is NULL.
 *
 * PARAMS
 *  hSrcMetaFile [I] handle of metafile to copy
 *  lpFilename   [I] filename if copying to a file
 *
 * RETURNS
 *  Handle to metafile copy on success, NULL on failure.
 *
 * BUGS
 *  Copying to disk returns NULL even if successful.
 */
HMETAFILE WINAPI CopyMetaFileW( HMETAFILE hSrcMetaFile, LPCWSTR lpFilename )
{
    METAHEADER *mh = MF_GetMetaHeader( hSrcMetaFile );
    METAHEADER *mh2 = NULL;
    HANDLE hFile;

    TRACE("(%p,%s)\n", hSrcMetaFile, debugstr_w(lpFilename));

    if(!mh) return 0;

    if(mh->mtType == METAFILE_DISK)
        mh2 = MF_LoadDiskBasedMetaFile(mh);
    else {
        mh2 = HeapAlloc( GetProcessHeap(), 0, mh->mtSize * 2 );
        memcpy( mh2, mh, mh->mtSize * 2 );
    }

    if(lpFilename) {         /* disk based metafile */
        DWORD w;
        if((hFile = CreateFileW(lpFilename, GENERIC_WRITE, 0, NULL,
				CREATE_ALWAYS, 0, 0)) == INVALID_HANDLE_VALUE) {
	    HeapFree( GetProcessHeap(), 0, mh2 );
	    return 0;
	}
	WriteFile(hFile, mh2, mh2->mtSize * 2, &w, NULL);
	CloseHandle(hFile);
    }

    return MF_Create_HMETAFILE( mh2 );
}


/******************************************************************
 *         CopyMetaFileA   (GDI32.@)
 *
 * See CopyMetaFileW.
 */
HMETAFILE WINAPI CopyMetaFileA( HMETAFILE hSrcMetaFile, LPCSTR lpFilename )
{
    UNICODE_STRING lpFilenameW;
    HMETAFILE ret = 0;

    if (lpFilename) RtlCreateUnicodeStringFromAsciiz(&lpFilenameW, lpFilename);
    else lpFilenameW.Buffer = NULL;

    ret = CopyMetaFileW( hSrcMetaFile, lpFilenameW.Buffer );
    if (lpFilenameW.Buffer)
        RtlFreeUnicodeString(&lpFilenameW);
    return ret;
}

/*******************************************************************
 *         MF_PlayMetaFile
 *
 * Helper for PlayMetaFile
 */
BOOL MF_PlayMetaFile( HDC hdc, METAHEADER *mh)
{

    METARECORD *mr;
    HANDLETABLE *ht;
    unsigned int offset = 0;
    WORD i;
    HPEN hPen;
    HBRUSH hBrush;
    HFONT hFont;
    HPALETTE hPal;
    HRGN hRgn;
    BOOL loaded = FALSE;

    if (!mh) return FALSE;
    if(mh->mtType == METAFILE_DISK) { /* Create a memory-based copy */
        mh = MF_LoadDiskBasedMetaFile(mh);
	if(!mh) return FALSE;
	loaded = TRUE;
    }

    /* save DC */
    hPen = GetCurrentObject(hdc, OBJ_PEN);
    hBrush = GetCurrentObject(hdc, OBJ_BRUSH);
    hFont = GetCurrentObject(hdc, OBJ_FONT);
    hPal = GetCurrentObject(hdc, OBJ_PAL);

    hRgn = CreateRectRgn(0, 0, 0, 0);
    if (!GetClipRgn(hdc, hRgn))
    {
        DeleteObject(hRgn);
        hRgn = 0;
    }

    /* create the handle table */
    ht = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
		    sizeof(HANDLETABLE) * mh->mtNoObjects);
    if(!ht) return FALSE;

    /* loop through metafile playing records */
    offset = mh->mtHeaderSize * 2;
    while (offset < mh->mtSize * 2)
    {
        mr = (METARECORD *)((char *)mh + offset);
	TRACE("offset=%04x,size=%08x\n",
            offset, mr->rdSize);
	if (mr->rdSize < 3) { /* catch illegal record sizes */
            TRACE("Entry got size %d at offset %d, total mf length is %d\n",
                  mr->rdSize,offset,mh->mtSize*2);
            break;
	}

	offset += mr->rdSize * 2;
	if (mr->rdFunction == META_EOF) {
	    TRACE("Got META_EOF so stopping\n");
	    break;
	}
	PlayMetaFileRecord( hdc, ht, mr, mh->mtNoObjects );
    }

    /* restore DC */
    SelectObject(hdc, hPen);
    SelectObject(hdc, hBrush);
    SelectPalette(hdc, hPal, FALSE);
    ExtSelectClipRgn(hdc, hRgn, RGN_COPY);
    DeleteObject(hRgn);

    /* free objects in handle table */
    for(i = 0; i < mh->mtNoObjects; i++)
      if(*(ht->objectHandle + i) != 0)
        DeleteObject(*(ht->objectHandle + i));

    /* free handle table */
    HeapFree( GetProcessHeap(), 0, ht );
    if(loaded)
        HeapFree( GetProcessHeap(), 0, mh );
    return TRUE;
}

/******************************************************************
 *         PlayMetaFile   (GDI32.@)
 *
 *  Renders the metafile specified by hmf in the DC specified by
 *  hdc. Returns FALSE on failure, TRUE on success.
 *
 * PARAMS
 *  hdc [I] handle of DC to render in
 *  hmf [I] handle of metafile to render
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI PlayMetaFile( HDC hdc, HMETAFILE hmf )
{
    METAHEADER *mh = MF_GetMetaHeader( hmf );
    return MF_PlayMetaFile( hdc, mh );
}

/******************************************************************
 *            EnumMetaFile   (GDI32.@)
 *
 *  Loop through the metafile records in hmf, calling the user-specified
 *  function for each one, stopping when the user's function returns FALSE
 *  (which is considered to be failure)
 *  or when no records are left (which is considered to be success).
 *
 * RETURNS
 *  TRUE on success, FALSE on failure.
 */
BOOL WINAPI EnumMetaFile(HDC hdc, HMETAFILE hmf, MFENUMPROC lpEnumFunc, LPARAM lpData)
{
    METAHEADER *mhTemp = NULL, *mh = MF_GetMetaHeader(hmf);
    METARECORD *mr;
    HANDLETABLE *ht;
    BOOL result = TRUE;
    int i;
    unsigned int offset = 0;
    HPEN hPen;
    HBRUSH hBrush;
    HFONT hFont;

    TRACE("(%p,%p,%p,%p)\n", hdc, hmf, lpEnumFunc, (void*)lpData);
    if (!mh) return 0;
    if(mh->mtType == METAFILE_DISK)
    {
        /* Create a memory-based copy */
        if (!(mhTemp = MF_LoadDiskBasedMetaFile(mh))) return FALSE;
	mh = mhTemp;
    }

    /* save the current pen, brush and font */
    hPen = GetCurrentObject(hdc, OBJ_PEN);
    hBrush = GetCurrentObject(hdc, OBJ_BRUSH);
    hFont = GetCurrentObject(hdc, OBJ_FONT);

    ht = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
			    sizeof(HANDLETABLE) * mh->mtNoObjects);

    /* loop through metafile records */
    offset = mh->mtHeaderSize * 2;

    while (offset < (mh->mtSize * 2))
    {
	mr = (METARECORD *)((char *)mh + offset);
	if(mr->rdFunction == META_EOF) {
	    TRACE("Got META_EOF so stopping\n");
	    break;
	}
	TRACE("Calling EnumFunc with record type %x\n",
	      mr->rdFunction);
        if (!lpEnumFunc( hdc, ht, mr, mh->mtNoObjects, (LONG)lpData ))
	{
	    result = FALSE;
	    break;
	}

	offset += (mr->rdSize * 2);
    }

    /* restore pen, brush and font */
    SelectObject(hdc, hBrush);
    SelectObject(hdc, hPen);
    SelectObject(hdc, hFont);

    /* free objects in handle table */
    for(i = 0; i < mh->mtNoObjects; i++)
      if(*(ht->objectHandle + i) != 0)
        DeleteObject(*(ht->objectHandle + i));

    /* free handle table */
    HeapFree( GetProcessHeap(), 0, ht);
    /* free a copy of metafile */
    HeapFree( GetProcessHeap(), 0, mhTemp );
    return result;
}

static BOOL MF_Play_MetaCreateRegion( METARECORD *mr, HRGN hrgn );
static BOOL MF_Play_MetaExtTextOut(HDC hdc, METARECORD *mr);
/******************************************************************
 *         PlayMetaFileRecord   (GDI32.@)
 *
 *   Render a single metafile record specified by *mr in the DC hdc, while
 *   using the handle table *ht, of length handles,
 *   to store metafile objects.
 *
 * BUGS
 *  The following metafile records are unimplemented:
 *
 *  DRAWTEXT, ANIMATEPALETTE, SETPALENTRIES,
 *  RESIZEPALETTE, EXTFLOODFILL, RESETDC, STARTDOC, STARTPAGE, ENDPAGE,
 *  ABORTDOC, ENDDOC, CREATEBRUSH, CREATEBITMAPINDIRECT, and CREATEBITMAP.
 */
BOOL WINAPI PlayMetaFileRecord( HDC hdc,  HANDLETABLE *ht, METARECORD *mr, UINT handles )
{
    short s1;
    POINT *pt;
    BITMAPINFOHEADER *infohdr;

    TRACE("(%p %p %p %u) function %04x\n", hdc, ht, mr, handles, mr->rdFunction);

    switch (mr->rdFunction)
    {
    case META_EOF:
        break;

    case META_DELETEOBJECT:
        DeleteObject(*(ht->objectHandle + mr->rdParm[0]));
        *(ht->objectHandle + mr->rdParm[0]) = 0;
        break;

    case META_SETBKCOLOR:
        SetBkColor(hdc, MAKELONG(mr->rdParm[0], mr->rdParm[1]));
        break;

    case META_SETBKMODE:
        SetBkMode(hdc, mr->rdParm[0]);
        break;

    case META_SETMAPMODE:
        SetMapMode(hdc, mr->rdParm[0]);
        break;

    case META_SETROP2:
        SetROP2(hdc, mr->rdParm[0]);
        break;

    case META_SETRELABS:
        SetRelAbs(hdc, mr->rdParm[0]);
        break;

    case META_SETPOLYFILLMODE:
        SetPolyFillMode(hdc, mr->rdParm[0]);
        break;

    case META_SETSTRETCHBLTMODE:
        SetStretchBltMode(hdc, mr->rdParm[0]);
        break;

    case META_SETTEXTCOLOR:
        SetTextColor(hdc, MAKELONG(mr->rdParm[0], mr->rdParm[1]));
        break;

    case META_SETWINDOWORG:
        SetWindowOrgEx(hdc, (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0], NULL);
        break;

    case META_SETWINDOWEXT:
        SetWindowExtEx(hdc, (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0], NULL);
        break;

    case META_SETVIEWPORTORG:
        SetViewportOrgEx(hdc, (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0], NULL);
        break;

    case META_SETVIEWPORTEXT:
        SetViewportExtEx(hdc, (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0], NULL);
        break;

    case META_OFFSETWINDOWORG:
        OffsetWindowOrgEx(hdc, (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0], NULL);
        break;

    case META_SCALEWINDOWEXT:
        ScaleWindowExtEx(hdc, (SHORT)mr->rdParm[3], (SHORT)mr->rdParm[2],
                              (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0], NULL);
        break;

    case META_OFFSETVIEWPORTORG:
        OffsetViewportOrgEx(hdc, (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0], NULL);
        break;

    case META_SCALEVIEWPORTEXT:
        ScaleViewportExtEx(hdc, (SHORT)mr->rdParm[3], (SHORT)mr->rdParm[2],
                                (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0], NULL);
        break;

    case META_LINETO:
        LineTo(hdc, (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0]);
        break;

    case META_MOVETO:
        MoveToEx(hdc, (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0], NULL);
        break;

    case META_EXCLUDECLIPRECT:
        ExcludeClipRect( hdc, (SHORT)mr->rdParm[3], (SHORT)mr->rdParm[2],
                              (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0] );
        break;

    case META_INTERSECTCLIPRECT:
        IntersectClipRect( hdc, (SHORT)mr->rdParm[3], (SHORT)mr->rdParm[2],
                                (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0] );
        break;

    case META_ARC:
        Arc(hdc, (SHORT)mr->rdParm[7], (SHORT)mr->rdParm[6],
                 (SHORT)mr->rdParm[5], (SHORT)mr->rdParm[4],
                 (SHORT)mr->rdParm[3], (SHORT)mr->rdParm[2],
                 (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0]);
        break;

    case META_ELLIPSE:
        Ellipse(hdc, (SHORT)mr->rdParm[3], (SHORT)mr->rdParm[2],
                     (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0]);
        break;

    case META_FLOODFILL:
        FloodFill(hdc, (SHORT)mr->rdParm[3], (SHORT)mr->rdParm[2],
                    MAKELONG(mr->rdParm[0], mr->rdParm[1]));
        break;

    case META_PIE:
        Pie(hdc, (SHORT)mr->rdParm[7], (SHORT)mr->rdParm[6],
                 (SHORT)mr->rdParm[5], (SHORT)mr->rdParm[4],
                 (SHORT)mr->rdParm[3], (SHORT)mr->rdParm[2],
                 (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0]);
        break;

    case META_RECTANGLE:
        Rectangle(hdc, (SHORT)mr->rdParm[3], (SHORT)mr->rdParm[2],
                       (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0]);
        break;

    case META_ROUNDRECT:
        RoundRect(hdc, (SHORT)mr->rdParm[5], (SHORT)mr->rdParm[4],
                       (SHORT)mr->rdParm[3], (SHORT)mr->rdParm[2],
                       (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0]);
        break;

    case META_PATBLT:
        PatBlt(hdc, (SHORT)mr->rdParm[5], (SHORT)mr->rdParm[4],
                    (SHORT)mr->rdParm[3], (SHORT)mr->rdParm[2],
                    MAKELONG(mr->rdParm[0], mr->rdParm[1]));
        break;

    case META_SAVEDC:
        SaveDC(hdc);
        break;

    case META_SETPIXEL:
        SetPixel(hdc, (SHORT)mr->rdParm[3], (SHORT)mr->rdParm[2],
                 MAKELONG(mr->rdParm[0], mr->rdParm[1]));
        break;

    case META_OFFSETCLIPRGN:
        OffsetClipRgn( hdc, (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0] );
        break;

    case META_TEXTOUT:
        s1 = mr->rdParm[0];
        TextOutA(hdc, (SHORT)mr->rdParm[((s1 + 1) >> 1) + 2],
                 (SHORT)mr->rdParm[((s1 + 1) >> 1) + 1],
                 (char *)(mr->rdParm + 1), s1);
        break;

    case META_POLYGON:
        if ((pt = convert_points( mr->rdParm[0], (LPPOINT16)(mr->rdParm + 1))))
        {
            Polygon(hdc, pt, mr->rdParm[0]);
            HeapFree( GetProcessHeap(), 0, pt );
        }
        break;

    case META_POLYPOLYGON:
        {
            UINT i, total;
            SHORT *counts = (SHORT *)(mr->rdParm + 1);

            for (i = total = 0; i < mr->rdParm[0]; i++) total += counts[i];
            pt = convert_points( total, (LPPOINT16)(counts + mr->rdParm[0]) );
            if (pt)
            {
                INT *cnt32 = HeapAlloc( GetProcessHeap(), 0, mr->rdParm[0] * sizeof(*cnt32) );
                if (cnt32)
                {
                    for (i = 0; i < mr->rdParm[0]; i++) cnt32[i] = counts[i];
                    PolyPolygon( hdc, pt, cnt32, mr->rdParm[0]);
                    HeapFree( GetProcessHeap(), 0, cnt32 );
                }
            }
            HeapFree( GetProcessHeap(), 0, pt );
        }
        break;

    case META_POLYLINE:
        if ((pt = convert_points( mr->rdParm[0], (LPPOINT16)(mr->rdParm + 1))))
        {
            Polyline( hdc, pt, mr->rdParm[0] );
            HeapFree( GetProcessHeap(), 0, pt );
        }
        break;

    case META_RESTOREDC:
        RestoreDC(hdc, (SHORT)mr->rdParm[0]);
        break;

    case META_SELECTOBJECT:
        SelectObject(hdc, *(ht->objectHandle + mr->rdParm[0]));
        break;

    case META_CHORD:
        Chord(hdc, (SHORT)mr->rdParm[7], (SHORT)mr->rdParm[6],
                   (SHORT)mr->rdParm[5], (SHORT)mr->rdParm[4],
                   (SHORT)mr->rdParm[3], (SHORT)mr->rdParm[2],
                   (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0]);
        break;

    case META_CREATEPATTERNBRUSH:
        switch (mr->rdParm[0])
        {
        case BS_PATTERN:
            infohdr = (BITMAPINFOHEADER *)(mr->rdParm + 2);
            MF_AddHandle(ht, handles,
                         CreatePatternBrush(CreateBitmap(infohdr->biWidth,
                                      infohdr->biHeight,
                                      infohdr->biPlanes,
                                      infohdr->biBitCount,
                                      (LPSTR)(mr->rdParm +
                                      (sizeof(BITMAPINFOHEADER) / 2) + 4))));
            break;

        case BS_DIBPATTERN:
            infohdr = (BITMAPINFOHEADER *)(mr->rdParm + 2);
            MF_AddHandle(ht, handles, CreateDIBPatternBrushPt( infohdr, mr->rdParm[1] ));
            break;

        default:
            ERR("META_CREATEPATTERNBRUSH: Unknown pattern type %d\n",
                mr->rdParm[0]);
            break;
        }
        break;

    case META_CREATEPENINDIRECT:
        {
            LOGPEN pen;
            pen.lopnStyle = mr->rdParm[0];
            pen.lopnWidth.x = (SHORT)mr->rdParm[1];
            pen.lopnWidth.y = (SHORT)mr->rdParm[2];
            pen.lopnColor = MAKELONG( mr->rdParm[3], mr->rdParm[4] );
            MF_AddHandle(ht, handles, CreatePenIndirect( &pen ));
        }
        break;

    case META_CREATEFONTINDIRECT:
        {
            LOGFONTA font;
            font.lfHeight         = (SHORT)mr->rdParm[0];
            font.lfWidth          = (SHORT)mr->rdParm[1];
            font.lfEscapement     = (SHORT)mr->rdParm[2];
            font.lfOrientation    = (SHORT)mr->rdParm[3];
            font.lfWeight         = (SHORT)mr->rdParm[4];
            font.lfItalic         = LOBYTE(mr->rdParm[5]);
            font.lfUnderline      = HIBYTE(mr->rdParm[5]);
            font.lfStrikeOut      = LOBYTE(mr->rdParm[6]);
            font.lfCharSet        = HIBYTE(mr->rdParm[6]);
            font.lfOutPrecision   = LOBYTE(mr->rdParm[7]);
            font.lfClipPrecision  = HIBYTE(mr->rdParm[7]);
            font.lfQuality        = LOBYTE(mr->rdParm[8]);
            font.lfPitchAndFamily = HIBYTE(mr->rdParm[8]);
            memcpy( font.lfFaceName, mr->rdParm + 9, LF_FACESIZE );
            MF_AddHandle(ht, handles, CreateFontIndirectA( &font ));
        }
        break;

    case META_CREATEBRUSHINDIRECT:
        {
            LOGBRUSH brush;
            brush.lbStyle = mr->rdParm[0];
            brush.lbColor = MAKELONG( mr->rdParm[1], mr->rdParm[2] );
            brush.lbHatch = mr->rdParm[3];
            MF_AddHandle(ht, handles, CreateBrushIndirect( &brush ));
        }
        break;

    case META_CREATEPALETTE:
        MF_AddHandle(ht, handles, CreatePalette((LPLOGPALETTE)mr->rdParm));
        break;

    case META_SETTEXTALIGN:
        SetTextAlign(hdc, mr->rdParm[0]);
        break;

    case META_SELECTPALETTE:
        GDISelectPalette(hdc, *(ht->objectHandle + mr->rdParm[1]), mr->rdParm[0]);
        break;

    case META_SETMAPPERFLAGS:
        SetMapperFlags(hdc, MAKELONG(mr->rdParm[0],mr->rdParm[1]));
        break;

    case META_REALIZEPALETTE:
        GDIRealizePalette(hdc);
        break;

    case META_ESCAPE:
        switch (mr->rdParm[0]) {
        case GETSCALINGFACTOR: /* get function ... would just NULL dereference */
        case GETPHYSPAGESIZE:
        case GETPRINTINGOFFSET:
             return FALSE;
        case SETABORTPROC:
             FIXME("Filtering Escape(SETABORTPROC), possible virus?\n");
             return FALSE;
        }
        Escape(hdc, mr->rdParm[0], mr->rdParm[1], (LPCSTR)&mr->rdParm[2], NULL);
        break;

    case META_EXTTEXTOUT:
        MF_Play_MetaExtTextOut( hdc, mr );
        break;

    case META_STRETCHDIB:
      {
        LPBITMAPINFO info = (LPBITMAPINFO) &(mr->rdParm[11]);
        LPSTR bits = (LPSTR)info + bitmap_info_size( info, mr->rdParm[2] );
        StretchDIBits( hdc, (SHORT)mr->rdParm[10], (SHORT)mr->rdParm[9], (SHORT)mr->rdParm[8],
                       (SHORT)mr->rdParm[7], (SHORT)mr->rdParm[6], (SHORT)mr->rdParm[5],
                       (SHORT)mr->rdParm[4], (SHORT)mr->rdParm[3], bits, info,
                       mr->rdParm[2],MAKELONG(mr->rdParm[0],mr->rdParm[1]));
      }
      break;

    case META_DIBSTRETCHBLT:
      {
        LPBITMAPINFO info = (LPBITMAPINFO) &(mr->rdParm[10]);
        LPSTR bits = (LPSTR)info + bitmap_info_size( info, mr->rdParm[2] );
        StretchDIBits( hdc, (SHORT)mr->rdParm[9], (SHORT)mr->rdParm[8], (SHORT)mr->rdParm[7],
                       (SHORT)mr->rdParm[6], (SHORT)mr->rdParm[5], (SHORT)mr->rdParm[4],
                       (SHORT)mr->rdParm[3], (SHORT)mr->rdParm[2], bits, info,
                       DIB_RGB_COLORS,MAKELONG(mr->rdParm[0],mr->rdParm[1]));
      }
      break;

    case META_STRETCHBLT:
      {
        HDC hdcSrc = CreateCompatibleDC(hdc);
        HBITMAP hbitmap = CreateBitmap(mr->rdParm[10], /*Width */
                                       mr->rdParm[11], /*Height*/
                                       mr->rdParm[13], /*Planes*/
                                       mr->rdParm[14], /*BitsPixel*/
                                       (LPSTR)&mr->rdParm[15]);  /*bits*/
        SelectObject(hdcSrc,hbitmap);
        StretchBlt(hdc, (SHORT)mr->rdParm[9], (SHORT)mr->rdParm[8],
                   (SHORT)mr->rdParm[7], (SHORT)mr->rdParm[6],
                   hdcSrc, (SHORT)mr->rdParm[5], (SHORT)mr->rdParm[4],
                   (SHORT)mr->rdParm[3], (SHORT)mr->rdParm[2],
                   MAKELONG(mr->rdParm[0],mr->rdParm[1]));
        DeleteDC(hdcSrc);
      }
      break;

    case META_BITBLT:
      {
        HDC hdcSrc = CreateCompatibleDC(hdc);
        HBITMAP hbitmap = CreateBitmap(mr->rdParm[7]/*Width */,
                                        mr->rdParm[8]/*Height*/,
                                        mr->rdParm[10]/*Planes*/,
                                        mr->rdParm[11]/*BitsPixel*/,
                                        (LPSTR)&mr->rdParm[12]/*bits*/);
        SelectObject(hdcSrc,hbitmap);
        BitBlt(hdc,(SHORT)mr->rdParm[6],(SHORT)mr->rdParm[5],
                (SHORT)mr->rdParm[4],(SHORT)mr->rdParm[3],
                hdcSrc, (SHORT)mr->rdParm[2],(SHORT)mr->rdParm[1],
                MAKELONG(0,mr->rdParm[0]));
        DeleteDC(hdcSrc);
      }
      break;

    case META_CREATEREGION:
      {
        HRGN hrgn = CreateRectRgn(0,0,0,0);

        MF_Play_MetaCreateRegion(mr, hrgn);
        MF_AddHandle(ht, handles, hrgn);
      }
      break;

    case META_FILLREGION:
        FillRgn(hdc, *(ht->objectHandle + mr->rdParm[1]),
                *(ht->objectHandle + mr->rdParm[0]));
        break;

    case META_FRAMEREGION:
        FrameRgn(hdc, *(ht->objectHandle + mr->rdParm[3]),
                 *(ht->objectHandle + mr->rdParm[2]),
                 (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0]);
        break;

    case META_INVERTREGION:
        InvertRgn(hdc, *(ht->objectHandle + mr->rdParm[0]));
        break;

    case META_PAINTREGION:
        PaintRgn(hdc, *(ht->objectHandle + mr->rdParm[0]));
        break;

    case META_SELECTCLIPREGION:
        {
            HRGN hrgn = 0;

            if (mr->rdParm[0]) hrgn = *(ht->objectHandle + mr->rdParm[0]);
            SelectClipRgn(hdc, hrgn);
        }
        break;

    case META_DIBCREATEPATTERNBRUSH:
        /*  mr->rdParm[0] may be BS_PATTERN or BS_DIBPATTERN:
            but there's no difference */
        MF_AddHandle(ht, handles, CreateDIBPatternBrushPt( mr->rdParm + 2, mr->rdParm[1] ));
        break;

    case META_DIBBITBLT:
      /* In practice I've found that there are two layouts for
         META_DIBBITBLT, one (the first here) is the usual one when a src
         dc is actually passed to it, the second occurs when the src dc is
         passed in as NULL to the creating BitBlt. As the second case has
         no dib, a size check will suffice to distinguish.

         Caolan.McNamara@ul.ie */

        if (mr->rdSize > 12) {
            LPBITMAPINFO info = (LPBITMAPINFO) &(mr->rdParm[8]);
            LPSTR bits = (LPSTR)info + bitmap_info_size(info, mr->rdParm[0]);

            StretchDIBits(hdc, (SHORT)mr->rdParm[7], (SHORT)mr->rdParm[6], (SHORT)mr->rdParm[5],
                          (SHORT)mr->rdParm[4], (SHORT)mr->rdParm[3], (SHORT)mr->rdParm[2],
                          (SHORT)mr->rdParm[5], (SHORT)mr->rdParm[4], bits, info,
                          DIB_RGB_COLORS, MAKELONG(mr->rdParm[0], mr->rdParm[1]));
        }
        else /* equivalent to a PatBlt */
            PatBlt(hdc, (SHORT)mr->rdParm[8], (SHORT)mr->rdParm[7],
                   (SHORT)mr->rdParm[6], (SHORT)mr->rdParm[5],
                   MAKELONG(mr->rdParm[0], mr->rdParm[1]));
        break;

    case META_SETTEXTCHAREXTRA:
        SetTextCharacterExtra(hdc, (SHORT)mr->rdParm[0]);
        break;

    case META_SETTEXTJUSTIFICATION:
        SetTextJustification(hdc, (SHORT)mr->rdParm[1], (SHORT)mr->rdParm[0]);
        break;

    case META_EXTFLOODFILL:
        ExtFloodFill(hdc, (SHORT)mr->rdParm[4], (SHORT)mr->rdParm[3],
                     MAKELONG(mr->rdParm[1], mr->rdParm[2]),
                     mr->rdParm[0]);
        break;

    case META_SETDIBTODEV:
        {
            BITMAPINFO *info = (BITMAPINFO *) &(mr->rdParm[9]);
            char *bits = (char *)info + bitmap_info_size( info, mr->rdParm[0] );
            SetDIBitsToDevice(hdc, (SHORT)mr->rdParm[8], (SHORT)mr->rdParm[7],
                              (SHORT)mr->rdParm[6], (SHORT)mr->rdParm[5],
                              (SHORT)mr->rdParm[4], (SHORT)mr->rdParm[3],
                              mr->rdParm[2], mr->rdParm[1], bits, info,
                              mr->rdParm[0]);
            break;
        }

#define META_UNIMP(x) case x: \
FIXME("PlayMetaFileRecord:record type "#x" not implemented.\n"); \
break;
    META_UNIMP(META_DRAWTEXT)
    META_UNIMP(META_ANIMATEPALETTE)
    META_UNIMP(META_SETPALENTRIES)
    META_UNIMP(META_RESIZEPALETTE)
    META_UNIMP(META_RESETDC)
    META_UNIMP(META_STARTDOC)
    META_UNIMP(META_STARTPAGE)
    META_UNIMP(META_ENDPAGE)
    META_UNIMP(META_ABORTDOC)
    META_UNIMP(META_ENDDOC)
    META_UNIMP(META_CREATEBRUSH)
    META_UNIMP(META_CREATEBITMAPINDIRECT)
    META_UNIMP(META_CREATEBITMAP)
#undef META_UNIMP

    default:
        WARN("PlayMetaFileRecord: Unknown record type %x\n", mr->rdFunction);
        return FALSE;
    }
    return TRUE;
}

/******************************************************************
 *         SetMetaFileBitsEx    (GDI32.@)
 *
 *  Create a metafile from raw data. No checking of the data is performed.
 *  Use GetMetaFileBitsEx() to get raw data from a metafile.
 *
 * PARAMS
 *  size   [I] size of metafile, in bytes
 *  lpData [I] pointer to metafile data
 *
 * RETURNS
 *  Success: Handle to metafile.
 *  Failure: NULL.
 */
HMETAFILE WINAPI SetMetaFileBitsEx( UINT size, const BYTE *lpData )
{
    const METAHEADER *mh_in = (const METAHEADER *)lpData;
    METAHEADER *mh_out;

    if (size & 1) return 0;

    if (!size || mh_in->mtType != METAFILE_MEMORY || mh_in->mtVersion != MFVERSION ||
        mh_in->mtHeaderSize != sizeof(METAHEADER) / 2)
    {
        SetLastError(ERROR_INVALID_DATA);
        return 0;
    }

    mh_out = HeapAlloc( GetProcessHeap(), 0, size );
    if (!mh_out)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    memcpy(mh_out, mh_in, size);
    mh_out->mtSize = size / 2;
    return MF_Create_HMETAFILE(mh_out);
}

/*****************************************************************
 *  GetMetaFileBitsEx     (GDI32.@)
 *
 * Get raw metafile data.
 *
 *  Copies the data from metafile _hmf_ into the buffer _buf_.
 *
 * PARAMS
 *  hmf   [I] metafile
 *  nSize [I] size of buf
 *  buf   [O] buffer to receive raw metafile data
 *
 * RETURNS
 *  If _buf_ is zero, returns size of buffer required. Otherwise,
 *  returns number of bytes copied.
 */
UINT WINAPI GetMetaFileBitsEx( HMETAFILE hmf, UINT nSize, LPVOID buf )
{
    METAHEADER *mh = MF_GetMetaHeader(hmf);
    UINT mfSize;

    TRACE("(%p,%d,%p)\n", hmf, nSize, buf);
    if (!mh) return 0;  /* FIXME: error code */
    if(mh->mtType == METAFILE_DISK)
        FIXME("Disk-based metafile?\n");
    mfSize = mh->mtSize * 2;
    if (!buf) {
	TRACE("returning size %d\n", mfSize);
	return mfSize;
    }
    if(mfSize > nSize) mfSize = nSize;
    memmove(buf, mh, mfSize);
    return mfSize;
}

/******************************************************************
 *         GetWinMetaFileBits [GDI32.@]
 */
UINT WINAPI GetWinMetaFileBits(HENHMETAFILE hemf,
                                UINT cbBuffer, LPBYTE lpbBuffer,
                                INT fnMapMode, HDC hdcRef)
{
    HDC hdcmf;
    HMETAFILE hmf;
    UINT ret;
    RECT rc;
    INT oldMapMode;

    GetClipBox(hdcRef, &rc);
    oldMapMode = SetMapMode(hdcRef, fnMapMode);

    TRACE("(%p,%d,%p,%d,%p) rc=%s\n", hemf, cbBuffer, lpbBuffer,
        fnMapMode, hdcRef, wine_dbgstr_rect(&rc));

    hdcmf = CreateMetaFileA(NULL);
    PlayEnhMetaFile(hdcmf, hemf, &rc);
    hmf = CloseMetaFile(hdcmf);
    ret = GetMetaFileBitsEx(hmf, cbBuffer, lpbBuffer);
    DeleteMetaFile(hmf);

    SetMapMode(hdcRef, oldMapMode);

    return ret;
}

/******************************************************************
 *         MF_Play_MetaCreateRegion
 *
 *  Handles META_CREATEREGION for PlayMetaFileRecord().
 *
 *	The layout of the record looks something like this:
 *
 *	 rdParm	meaning
 *	 0		Always 0?
 *	 1		Always 6?
 *	 2		Looks like a handle? - not constant
 *	 3		0 or 1 ??
 *	 4		Total number of bytes
 *	 5		No. of separate bands = n [see below]
 *	 6		Largest number of x co-ords in a band
 *	 7-10		Bounding box x1 y1 x2 y2
 *	 11-...		n bands
 *
 *	 Regions are divided into bands that are uniform in the
 *	 y-direction. Each band consists of pairs of on/off x-coords and is
 *	 written as
 *		m y0 y1 x1 x2 x3 ... xm m
 *	 into successive rdParm[]s.
 *
 *	 This is probably just a dump of the internal RGNOBJ?
 *
 *	 HDMD - 18/12/97
 *
 */

static BOOL MF_Play_MetaCreateRegion( METARECORD *mr, HRGN hrgn )
{
    WORD band, pair;
    WORD *start, *end;
    INT16 y0, y1;
    HRGN hrgn2 = CreateRectRgn( 0, 0, 0, 0 );

    for(band  = 0, start = &(mr->rdParm[11]); band < mr->rdParm[5];
 					        band++, start = end + 1) {
        if(*start / 2 != (*start + 1) / 2) {
 	    WARN("Delimiter not even.\n");
	    DeleteObject( hrgn2 );
 	    return FALSE;
        }

	end = start + *start + 3;
	if(end > (WORD *)mr + mr->rdSize) {
	    WARN("End points outside record.\n");
	    DeleteObject( hrgn2 );
	    return FALSE;
        }

	if(*start != *end) {
	    WARN("Mismatched delimiters.\n");
	    DeleteObject( hrgn2 );
	    return FALSE;
	}

	y0 = *(INT16 *)(start + 1);
	y1 = *(INT16 *)(start + 2);
	for(pair = 0; pair < *start / 2; pair++) {
	    SetRectRgn( hrgn2, *(INT16 *)(start + 3 + 2*pair), y0,
				 *(INT16 *)(start + 4 + 2*pair), y1 );
	    CombineRgn(hrgn, hrgn, hrgn2, RGN_OR);
        }
    }
    DeleteObject( hrgn2 );
    return TRUE;
 }


/******************************************************************
 *         MF_Play_MetaExtTextOut
 *
 *  Handles META_EXTTEXTOUT for PlayMetaFileRecord().
 */

static BOOL MF_Play_MetaExtTextOut(HDC hdc, METARECORD *mr)
{
    INT *dx = NULL;
    int i;
    LPINT16 dxx;
    LPSTR sot;
    DWORD len;
    WORD s1;
    RECT rect;
    BOOL isrect = mr->rdParm[3] & (ETO_OPAQUE | ETO_CLIPPED);

    s1 = mr->rdParm[2];                              /* String length */
    len = sizeof(METARECORD) + (((s1 + 1) >> 1) * 2) + 2 * sizeof(short)
      + sizeof(UINT16) + (isrect ? sizeof(RECT16) : 0);
                                           /* rec len without dx array */

    sot = (LPSTR)&mr->rdParm[4];		      /* start_of_text */
    if (isrect)
    {
        rect.left   = (SHORT)mr->rdParm[4];
        rect.top    = (SHORT)mr->rdParm[5];
        rect.right  = (SHORT)mr->rdParm[6];
        rect.bottom = (SHORT)mr->rdParm[7];
        sot += sizeof(RECT16);  /* there is a rectangle, so add offset */
    }

    if (mr->rdSize == len / 2)
        dxx = NULL;                      /* determine if array is present */
    else
        if (mr->rdSize == (len + s1 * sizeof(INT16)) / 2)
        {
            dxx = (LPINT16)(sot+(((s1+1)>>1)*2));
            dx = HeapAlloc( GetProcessHeap(), 0, s1*sizeof(INT));
            if (dx) for (i = 0; i < s1; i++) dx[i] = dxx[i];
        }
	else {
            TRACE("%s  len: %d\n",  sot, mr->rdSize);
            WARN("Please report: ExtTextOut len=%d slen=%d rdSize=%d opt=%04x\n",
		 len, s1, mr->rdSize, mr->rdParm[3]);
	    dxx = NULL; /* shouldn't happen -- but if, we continue with NULL */
	}
    ExtTextOutA( hdc,
                 (SHORT)mr->rdParm[1],       /* X position */
                 (SHORT)mr->rdParm[0],       /* Y position */
                 mr->rdParm[3],              /* options */
                 &rect,                      /* rectangle */
                 sot,                        /* string */
                 s1, dx);                    /* length, dx array */
    if (dx)
    {
        TRACE("%s  len: %d  dx0: %d\n", sot, mr->rdSize, dx[0]);
        HeapFree( GetProcessHeap(), 0, dx );
    }
    return TRUE;
}

/*
 * Copyright (C) the Wine project
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

#ifndef __WINE_SHLOBJ_H
#define __WINE_SHLOBJ_H

#include <ole2.h>
#include <commctrl.h>
#include <prsht.h>
#include <shlguid.h>

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/* Except for specific structs, this header is byte packed */
#include <pshpack1.h>

#include <shtypes.h>
#include <shobjidl.h>

#ifndef HPSXA_DEFINED
#define HPSXA_DEFINED
DECLARE_HANDLE(HPSXA);
#endif

UINT         WINAPI SHAddFromPropSheetExtArray(HPSXA,LPFNADDPROPSHEETPAGE,LPARAM);
LPVOID       WINAPI SHAlloc(ULONG);
HRESULT      WINAPI SHCoCreateInstance(LPCWSTR,const CLSID*,IUnknown*,REFIID,LPVOID*);
HPSXA        WINAPI SHCreatePropSheetExtArray(HKEY,LPCWSTR,UINT);
HPSXA        WINAPI SHCreatePropSheetExtArrayEx(HKEY,LPCWSTR,UINT,IDataObject*);
DWORD        WINAPI SHCLSIDFromStringA(LPCSTR,CLSID*);
DWORD        WINAPI SHCLSIDFromStringW(LPCWSTR,CLSID*);
#define             SHCLSIDFromString WINELIB_NAME_AW(SHCLSIDFromString)
HRESULT      WINAPI SHCreateStdEnumFmtEtc(DWORD,const FORMATETC *,IEnumFORMATETC**);
void         WINAPI SHDestroyPropSheetExtArray(HPSXA);
BOOL         WINAPI SHFindFiles(LPCITEMIDLIST,LPCITEMIDLIST);
DWORD        WINAPI SHFormatDrive(HWND,UINT,UINT,UINT);
void         WINAPI SHFree(LPVOID);
BOOL         WINAPI GetFileNameFromBrowse(HWND,LPSTR,DWORD,LPCSTR,LPCSTR,LPCSTR,LPCSTR);
HRESULT      WINAPI SHGetInstanceExplorer(IUnknown**);
HRESULT      WINAPI SHGetFolderPathAndSubDirA(HWND,int,HANDLE,DWORD,LPCSTR,LPSTR);
HRESULT      WINAPI SHGetFolderPathAndSubDirW(HWND,int,HANDLE,DWORD,LPCWSTR,LPWSTR);
#define             SHGetFolderPathAndSubDir WINELIB_NAME_AW(SHGetFolderPathAndSubDir);
BOOL         WINAPI SHGetPathFromIDListA(LPCITEMIDLIST,LPSTR);
BOOL         WINAPI SHGetPathFromIDListW(LPCITEMIDLIST,LPWSTR);
#define             SHGetPathFromIDList WINELIB_NAME_AW(SHGetPathFromIDList)
INT          WINAPI SHHandleUpdateImage(LPCITEMIDLIST);
HRESULT      WINAPI SHILCreateFromPath(LPCWSTR,LPITEMIDLIST*,DWORD*);
HRESULT      WINAPI SHLoadOLE(LPARAM);
HRESULT      WINAPI SHPathPrepareForWriteA(HWND,IUnknown*,LPCSTR,DWORD);
HRESULT      WINAPI SHPathPrepareForWriteW(HWND,IUnknown*,LPCWSTR,DWORD);
#define             SHPathPrepareForWrite WINELIB_NAME_AW(SHPathPrepareForWrite);
UINT         WINAPI SHReplaceFromPropSheetExtArray(HPSXA,UINT,LPFNADDPROPSHEETPAGE,LPARAM);
LPITEMIDLIST WINAPI SHSimpleIDListFromPath(LPCWSTR);
int          WINAPI SHMapPIDLToSystemImageListIndex(IShellFolder*,LPCITEMIDLIST,int*);
HRESULT      WINAPI SHStartNetConnectionDialog(HWND,LPCSTR,DWORD);
VOID         WINAPI SHUpdateImageA(LPCSTR,INT,UINT,INT);
VOID         WINAPI SHUpdateImageW(LPCWSTR,INT,UINT,INT);
#define             SHUpdateImage WINELIB_NAME_AW(SHUpdateImage)
int          WINAPI RestartDialog(HWND,LPCWSTR,DWORD);
int          WINAPI RestartDialogEx(HWND,LPCWSTR,DWORD,DWORD);

#define SHFMT_ERROR     0xFFFFFFFFL  /* Error on last format, drive may be formattable */
#define SHFMT_CANCEL    0xFFFFFFFEL  /* Last format was cancelled */
#define SHFMT_NOFORMAT  0xFFFFFFFDL  /* Drive is not formattable */

/* SHFormatDrive flags */
#define SHFMT_ID_DEFAULT	0xFFFF
#define SHFMT_OPT_FULL		1
#define SHFMT_OPT_SYSONLY	2

/* SHPathPrepareForWrite flags */
#define SHPPFW_NONE             0x00000000
#define SHPPFW_DIRCREATE        0x00000001
#define SHPPFW_DEFAULT          SHPPFW_DIRCREATE
#define SHPPFW_ASKDIRCREATE     0x00000002
#define SHPPFW_IGNOREFILENAME   0x00000004
#define SHPPFW_NOWRITECHECK     0x00000008
#define SHPPFW_MEDIACHECKONLY   0x00000010

/* SHObjectProperties flags */
#define SHOP_PRINTERNAME 0x01
#define SHOP_FILEPATH    0x02
#define SHOP_VOLUMEGUID  0x04

BOOL WINAPI SHObjectProperties(HWND,DWORD,LPCWSTR,LPCWSTR);

#define PCS_FATAL           0x80000000
#define PCS_REPLACEDCHAR    0x00000001
#define PCS_REMOVEDCHAR     0x00000002
#define PCS_TRUNCATED       0x00000004
#define PCS_PATHTOOLONG     0x00000008

int WINAPI PathCleanupSpec(LPCWSTR,LPWSTR);

/*****************************************************************************
 * IContextMenu interface
 */


/* DATAOBJECT_InitShellIDList*/
#define CFSTR_SHELLIDLISTA           "Shell IDList Array"   /* CF_IDLIST */
#define CFSTR_SHELLIDLISTOFFSETA     "Shell Object Offsets" /* CF_OBJECTPOSITIONS */
#define CFSTR_NETRESOURCESA          "Net Resource"         /* CF_NETRESOURCE */
/* DATAOBJECT_InitFileGroupDesc */
#define CFSTR_FILEDESCRIPTORA        "FileGroupDescriptor"  /* CF_FILEGROUPDESCRIPTORA */
/* DATAOBJECT_InitFileContents*/
#define CFSTR_FILECONTENTSA          "FileContents"         /* CF_FILECONTENTS */
#define CFSTR_FILENAMEA              "FileName"             /* CF_FILENAMEA */
#define CFSTR_FILENAMEMAPA           "FileNameMap"          /* CF_FILENAMEMAPA */
#define CFSTR_PRINTERGROUPA          "PrinterFriendlyName"  /* CF_PRINTERS */
#define CFSTR_SHELLURLA              "UniformResourceLocator"
#define CFSTR_INETURLA               CFSTR_SHELLURLA
#define CFSTR_PREFERREDDROPEFFECTA   "Preferred DropEffect"
#define CFSTR_PERFORMEDDROPEFFECTA   "Performed DropEffect"
#define CFSTR_PASTESUCCEEDEDA        "Paste Succeeded"
#define CFSTR_INDRAGLOOPA            "InShellDragLoop"
#define CFSTR_DRAGCONTEXTA           "DragContext"
#define CFSTR_MOUNTEDVOLUMEA         "MountedVolume"
#define CFSTR_PERSISTEDDATAOBJECTA   "PersistedDataObject"
#define CFSTR_TARGETCLSIDA           "TargetCLSID"
#define CFSTR_AUTOPLAY_SHELLIDLISTSA "Autoplay Enumerated IDList Array"
#define CFSTR_LOGICALPERFORMEDDROPEFFECTA "Logical Performed DropEffect"

#if defined(__GNUC__)
# define CFSTR_SHELLIDLISTW \
    (const WCHAR []){ 'S','h','e','l','l',' ','I','D','L','i','s','t',' ','A','r','r','a','y',0 }
# define CFSTR_SHELLIDLISTOFFSETW \
    (const WCHAR []){ 'S','h','e','l','l',' ','O','b','j','e','c','t',' ','O','f','f','s','e','t','s',0 }
# define CFSTR_NETRESOURCESW \
    (const WCHAR []){ 'N','e','t',' ','R','e','s','o','u','r','c','e',0 }
# define CFSTR_FILEDESCRIPTORW \
    (const WCHAR []){ 'F','i','l','e','G','r','o','u','p','D','e','s','c','r','i','p','t','o','r','W',0 }
# define CFSTR_FILECONTENTSW \
    (const WCHAR []){ 'F','i','l','e','C','o','n','t','e','n','t','s',0 }
# define CFSTR_FILENAMEW \
    (const WCHAR []){ 'F','i','l','e','N','a','m','e','W',0 }
# define CFSTR_FILENAMEMAPW \
    (const WCHAR []){ 'F','i','l','e','N','a','m','e','M','a','p','W',0 }
# define CFSTR_PRINTERGROUPW \
    (const WCHAR []){ 'P','r','i','n','t','e','r','F','r','i','e','n','d','l','y','N','a','m','e',0 }
# define CFSTR_SHELLURLW \
    (const WCHAR []){ 'U','n','i','f','o','r','m','R','e','s','o','u','r','c','e','L','o','c','a','t','o','r',0 }
# define CFSTR_INETURLW \
    (const WCHAR []){ 'U','n','i','f','o','r','m','R','e','s','o','u','r','c','e','L','o','c','a','t','o','r','W',0 }
# define CFSTR_PREFERREDDROPEFFECTW \
    (const WCHAR []){ 'P','r','e','f','e','r','r','e','d',' ','D','r','o','p','E','f','f','e','c','t',0 }
# define CFSTR_PERFORMEDDROPEFFECTW \
    (const WCHAR []){ 'P','e','r','f','o','r','m','e','d',' ','D','r','o','p','E','f','f','e','c','t',0 }
# define CFSTR_PASTESUCCEEDEDW \
    (const WCHAR []){ 'P','a','s','t','e',' ','S','u','c','c','e','e','d','e','d',0 }
# define CFSTR_INDRAGLOOPW \
    (const WCHAR []){ 'I','n','S','h','e','l','l','D','r','a','g','L','o','o','p',0 }
# define CFSTR_DRAGCONTEXTW \
    (const WCHAR []){ 'D','r','a','g','C','o','n','t','e','x','t',0 }
# define CFSTR_MOUNTEDVOLUMEW \
    (const WCHAR []){ 'M','o','u','n','t','e','d','V','o','l','u','m','e',0 }
# define CFSTR_PERSISTEDDATAOBJECTW \
    (const WCHAR []){ 'P','e','r','s','i','s','t','e','d','D','a','t','a','O','b','j','e','c','t',0 }
# define CFSTR_TARGETCLSIDW \
    (const WCHAR []){ 'T','a','r','g','e','t','C','L','S','I','D',0 }
# define CFSTR_AUTOPLAY_SHELLIDLISTSW \
    (const WCHAR []){ 'A','u','t','o','p','l','a','y',' ','E','n','u','m','e','r','a','t','e','d',\
                      ' ','I','D','L','i','s','t',' ','A','r','r','a','y',0 }
# define CFSTR_LOGICALPERFORMEDDROPEFFECTW \
    (const WCHAR []){ 'L','o','g','i','c','a','l',' ','P','e','r','f','o','r','m','e','d',\
                      ' ','D','r','o','p','E','f','f','e','c','t',0 }
#elif defined(_MSC_VER)
# define CFSTR_SHELLIDLISTW           L"Shell IDList Array"
# define CFSTR_SHELLIDLISTOFFSETW     L"Shell Object Offsets"
# define CFSTR_NETRESOURCESW          L"Net Resource"
# define CFSTR_FILEDESCRIPTORW        L"FileGroupDescriptorW"
# define CFSTR_FILECONTENTSW          L"FileContents"
# define CFSTR_FILENAMEW              L"FileNameW"
# define CFSTR_FILENAMEMAPW           L"FileNameMapW"
# define CFSTR_PRINTERGROUPW          L"PrinterFriendlyName"
# define CFSTR_SHELLURLW              L"UniformResourceLocator"
# define CFSTR_INETURLW               L"UniformResourceLocatorW"
# define CFSTR_PREFERREDDROPEFFECTW   L"Preferred DropEffect"
# define CFSTR_PERFORMEDDROPEFFECTW   L"Performed DropEffect"
# define CFSTR_PASTESUCCEEDEDW        L"Paste Succeeded"
# define CFSTR_INDRAGLOOPW            L"InShellDragLoop"
# define CFSTR_DRAGCONTEXTW           L"DragContext"
# define CFSTR_MOUNTEDVOLUMEW         L"MountedVolume"
# define CFSTR_PERSISTEDDATAOBJECTW   L"PersistedDataObject"
# define CFSTR_TARGETCLSIDW           L"TargetCLSID"
# define CFSTR_AUTOPLAY_SHELLIDLISTSW L"Autoplay Enumerated IDList Array"
# define CFSTR_LOGICALPERFORMEDDROPEFFECTW L"Logical Performed DropEffect"
#else
static const WCHAR CFSTR_SHELLIDLISTW[] =
    { 'S','h','e','l','l',' ','I','D','L','i','s','t',' ','A','r','r','a','y',0 };
static const WCHAR CFSTR_SHELLIDLISTOFFSETW[] =
    { 'S','h','e','l','l',' ','O','b','j','e','c','t',' ','O','f','f','s','e','t','s',0 };
static const WCHAR CFSTR_NETRESOURCESW[] =
    { 'N','e','t',' ','R','e','s','o','u','r','c','e',0 };
static const WCHAR CFSTR_FILEDESCRIPTORW[] =
    { 'F','i','l','e','G','r','o','u','p','D','e','s','c','r','i','p','t','o','r','W',0 };
static const WCHAR CFSTR_FILECONTENTSW[] =
    { 'F','i','l','e','C','o','n','t','e','n','t','s',0 };
static const WCHAR CFSTR_FILENAMEW[] =
    { 'F','i','l','e','N','a','m','e','W',0 }
static const WCHAR CFSTR_FILENAMEMAPW[] =
    { 'F','i','l','e','N','a','m','e','M','a','p','W',0 };
static const WCHAR CFSTR_PRINTERGROUPW[] =
    { 'P','r','i','n','t','e','r','F','r','i','e','n','d','l','y','N','a','m','e',0 };
static const WCHAR CFSTR_SHELLURLW[] =
    { 'U','n','i','f','o','r','m','R','e','s','o','u','r','c','e','L','o','c','a','t','o','r',0 };
static const WCHAR CFSTR_INETURLW[] =
    { 'U','n','i','f','o','r','m','R','e','s','o','u','r','c','e','L','o','c','a','t','o','r','W',0 };
static const WCHAR CFSTR_PREFERREDDROPEFFECTW[] =
    { 'P','r','e','f','e','r','r','e','d',' ','D','r','o','p','E','f','f','e','c','t',0 };
static const WCHAR CFSTR_PERFORMEDDROPEFFECTW[] =
    { 'P','e','r','f','o','r','m','e','d',' ','D','r','o','p','E','f','f','e','c','t',0 };
static const WCHAR CFSTR_PASTESUCCEEDEDW[] =
    { 'P','a','s','t','e',' ','S','u','c','c','e','e','d','e','d',0 };
static const WCHAR CFSTR_INDRAGLOOPW[] =
    { 'I','n','S','h','e','l','l','D','r','a','g','L','o','o','p',0 };
static const WCHAR CFSTR_DRAGCONTEXTW[] =
    { 'D','r','a','g','C','o','n','t','e','x','t',0 };
static const WCHAR CFSTR_MOUNTEDVOLUMEW[] =
    { 'M','o','u','n','t','e','d','V','o','l','u','m','e',0 };
static const WCHAR CFSTR_PERSISTEDDATAOBJECTW[] =
    { 'P','e','r','s','i','s','t','e','d','D','a','t','a','O','b','j','e','c','t',0 };
static const WCHAR CFSTR_TARGETCLSIDW[] =
    { 'T','a','r','g','e','t','C','L','S','I','D',0 };
static const WCHAR CFSTR_AUTOPLAY_SHELLIDLISTSW[] =
    { 'A','u','t','o','p','l','a','y',' ','E','n','u','m','e','r','a','t','e','d',
      ' ','I','D','L','i','s','t',' ','A','r','r','a','y',0 };
static const WCHAR CFSTR_LOGICALPERFORMEDDROPEFFECTW[] =
    { 'L','o','g','i','c','a','l',' ','P','e','r','f','o','r','m','e','d',
      ' ','D','r','o','p','E','f','f','e','c','t',0 };
#endif

#define CFSTR_SHELLIDLIST           WINELIB_NAME_AW(CFSTR_SHELLIDLIST)
#define CFSTR_SHELLIDLISTOFFSET     WINELIB_NAME_AW(CFSTR_SHELLIDLISTOFFSET)
#define CFSTR_NETRESOURCES          WINELIB_NAME_AW(CFSTR_NETRESOURCES)
#define CFSTR_FILEDESCRIPTOR        WINELIB_NAME_AW(CFSTR_FILEDESCRIPTOR)
#define CFSTR_FILECONTENTS          WINELIB_NAME_AW(CFSTR_FILECONTENTS)
#define CFSTR_FILENAME              WINELIB_NAME_AW(CFSTR_FILENAME)
#define CFSTR_FILENAMEMAP           WINELIB_NAME_AW(CFSTR_FILENAMEMAP)
#define CFSTR_PRINTERGROUP          WINELIB_NAME_AW(CFSTR_PRINTERGROUP)
#define CFSTR_SHELLURL              WINELIB_NAME_AW(CFSTR_SHELLURL)
#define CFSTR_INETURL               WINELIB_NAME_AW(CFSTR_INETURL)
#define CFSTR_PREFERREDDROPEFFECT   WINELIB_NAME_AW(CFSTR_PREFERREDDROPEFFECT)
#define CFSTR_PERFORMEDDROPEFFECT   WINELIB_NAME_AW(CFSTR_PERFORMEDDROPEFFECT)
#define CFSTR_PASTESUCCEEDED        WINELIB_NAME_AW(CFSTR_PASTESUCCEEDED)
#define CFSTR_INDRAGLOOP            WINELIB_NAME_AW(CFSTR_INDRAGLOOP)
#define CFSTR_DRAGCONTEXT           WINELIB_NAME_AW(CFSTR_DRAGCONTEXT)
#define CFSTR_MOUNTEDVOLUME         WINELIB_NAME_AW(CFSTR_MOUNTEDVOLUME)
#define CFSTR_PERSISTEDDATAOBJECT   WINELIB_NAME_AW(CFSTR_PERSISTEDDATAOBJECT)
#define CFSTR_TARGETCLSID           WINELIB_NAME_AW(CFSTR_TARGETCLSID)
#define CFSTR_AUTOPLAY_SHELLIDLISTS WINELIB_NAME_AW(CFSTR_AUTOPLAY_SHELLIDLISTS)
#define CFSTR_LOGICALPERFORMEDDROPEFFECT WINELIB_NAME_AW(CFSTR_LOGICALPERFORMEDDROPEFFECT)

typedef struct
{	UINT cidl;
	UINT aoffset[1];
} CIDA, *LPIDA;

/************************************************************************
* IShellView interface
*/

typedef GUID SHELLVIEWID;

#define FCIDM_SHVIEWFIRST       0x0000
/* undocumented */
#define FCIDM_SHVIEW_ARRANGE    0x7001
#define FCIDM_SHVIEW_DELETE     0x7011
#define FCIDM_SHVIEW_PROPERTIES 0x7013
#define FCIDM_SHVIEW_CUT        0x7018
#define FCIDM_SHVIEW_COPY       0x7019
#define FCIDM_SHVIEW_INSERT     0x701A
#define FCIDM_SHVIEW_UNDO       0x701B
#define FCIDM_SHVIEW_INSERTLINK 0x701C
#define FCIDM_SHVIEW_SELECTALL  0x7021
#define FCIDM_SHVIEW_INVERTSELECTION 0x7022

#define FCIDM_SHVIEW_BIGICON    0x7029
#define FCIDM_SHVIEW_SMALLICON  0x702A
#define FCIDM_SHVIEW_LISTVIEW   0x702B
#define FCIDM_SHVIEW_REPORTVIEW 0x702C
/* 0x7030-0x703f are used by the shellbrowser */
#define FCIDM_SHVIEW_AUTOARRANGE 0x7031
#define FCIDM_SHVIEW_SNAPTOGRID 0x7032

#define FCIDM_SHVIEW_HELP       0x7041
#define FCIDM_SHVIEW_RENAME     0x7050
#define FCIDM_SHVIEW_CREATELINK 0x7051
#define FCIDM_SHVIEW_NEWLINK    0x7052
#define FCIDM_SHVIEW_NEWFOLDER  0x7053

#define FCIDM_SHVIEW_REFRESH    0x7100 /* FIXME */
#define FCIDM_SHVIEW_EXPLORE    0x7101 /* FIXME */
#define FCIDM_SHVIEW_OPEN       0x7102 /* FIXME */

#define FCIDM_SHVIEWLAST        0x7fff
#define FCIDM_BROWSERFIRST      0xA000
/* undocumented toolbar items from stddlg's*/
#define FCIDM_TB_UPFOLDER       0xA001
#define FCIDM_TB_NEWFOLDER      0xA002
#define FCIDM_TB_SMALLICON      0xA003
#define FCIDM_TB_REPORTVIEW     0xA004
#define FCIDM_TB_DESKTOP        0xA005  /* FIXME */

#define FCIDM_BROWSERLAST       0xbf00
#define FCIDM_GLOBALFIRST       0x8000
#define FCIDM_GLOBALLAST        0x9fff

/*
* Global submenu IDs and separator IDs
*/
#define FCIDM_MENU_FILE             (FCIDM_GLOBALFIRST+0x0000)
#define FCIDM_MENU_EDIT             (FCIDM_GLOBALFIRST+0x0040)
#define FCIDM_MENU_VIEW             (FCIDM_GLOBALFIRST+0x0080)
#define FCIDM_MENU_VIEW_SEP_OPTIONS (FCIDM_GLOBALFIRST+0x0081)
#define FCIDM_MENU_TOOLS            (FCIDM_GLOBALFIRST+0x00c0)
#define FCIDM_MENU_TOOLS_SEP_GOTO   (FCIDM_GLOBALFIRST+0x00c1)
#define FCIDM_MENU_HELP             (FCIDM_GLOBALFIRST+0x0100)
#define FCIDM_MENU_FIND             (FCIDM_GLOBALFIRST+0x0140)
#define FCIDM_MENU_EXPLORE          (FCIDM_GLOBALFIRST+0x0150)
#define FCIDM_MENU_FAVORITES        (FCIDM_GLOBALFIRST+0x0170)

/* control IDs known to the view */
#define FCIDM_TOOLBAR      (FCIDM_BROWSERFIRST + 0)
#define FCIDM_STATUS       (FCIDM_BROWSERFIRST + 1)


/****************************************************************************
 * IShellIcon interface
 */

#define INTERFACE IShellIcon
DECLARE_INTERFACE_(IShellIcon,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IShellIcon methods ***/
    STDMETHOD(GetIconOf)(THIS_ LPCITEMIDLIST pidl, UINT flags, LPINT lpIconIndex) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IShellIcon_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IShellIcon_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IShellIcon_Release(p)                 (p)->lpVtbl->Release(p)
/*** IShellIcon methods ***/
#define IShellIcon_GetIconOf(p,a,b,c)         (p)->lpVtbl->GetIconOf(p,a,b,c)
#endif

typedef IShellIcon *LPSHELLICON;

/* IQueryInfo interface */
#define INTERFACE IQueryInfo
DECLARE_INTERFACE_(IQueryInfo,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IQueryInfo methods ***/
    STDMETHOD(GetInfoTip)(THIS_ DWORD dwFlags, WCHAR** lppTips) PURE;
    STDMETHOD(GetInfoFlags)(THIS_ DWORD* lpFlags) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IQueryInfo_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IQueryInfo_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IQueryInfo_Release(p)                 (p)->lpVtbl->Release(p)
/*** IQueryInfo methods ***/
#define IQueryInfo_GetInfoTip(p,a,b)          (p)->lpVtbl->GetInfoTip(p,a,b)
#define IQueryInfo_GetInfoFlags(p,a)          (p)->lpVtbl->GetInfoFlags(p,a)
#endif

/* IInputObject interface */
#define INTERFACE IInputObject
DECLARE_INTERFACE_(IInputObject,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IInputObject methods ***/
    STDMETHOD(UIActivateIO)(THIS_ BOOL bActivating, LPMSG lpMsg) PURE;
    STDMETHOD(HasFocusIO)(THIS) PURE;
    STDMETHOD(TranslateAcceleratorIO)(THIS_ LPMSG lpMsg) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IInputObject_QueryInterface(p,a,b)       (p)->lpVtbl->QueryInterface(p,a,b)
#define IInputObject_AddRef(p)                   (p)->lpVtbl->AddRef(p)
#define IInputObject_Release(p)                  (p)->lpVtbl->Release(p)
/*** IInputObject methods ***/
#define IInputObject_UIActivateIO(p,a,b)         (p)->lpVtbl->UIActivateIO(p,a,b)
#define IInputObject_HasFocusIO(p)               (p)->lpVtbl->HasFocusIO(p)
#define IInputObject_TranslateAcceleratorIO(p,a) (p)->lpVtbl->TranslateAcceleratorIO(p,a)
#endif

/* IInputObjectSite interface */
#define INTERFACE IInputObjectSite
DECLARE_INTERFACE_(IInputObjectSite,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface) (THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IInputObjectSite methods ***/
    STDMETHOD(OnFocusChangeIS)(THIS_ LPUNKNOWN lpUnknown, BOOL bFocus) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IInputObjectSite_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IInputObjectSite_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IInputObjectSite_Release(p)             (p)->lpVtbl->Release(p)
/*** IInputObject methods ***/
#define IInputObjectSite_OnFocusChangeIS(p,a,b) (p)->lpVtbl->OnFocusChangeIS(p,a,b)
#endif

/* IObjMgr interface */
#define INTERFACE IObjMgr
DECLARE_INTERFACE_(IObjMgr,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface) (THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IObjMgr methods ***/
    STDMETHOD(Append)(THIS_ LPUNKNOWN punk) PURE;
    STDMETHOD(Remove)(THIS_ LPUNKNOWN punk) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IObjMgr_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IObjMgr_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IObjMgr_Release(p)             (p)->lpVtbl->Release(p)
/*** IObjMgr methods ***/
#define IObjMgr_Append(p,a) (p)->lpVtbl->Append(p,a)
#define IObjMgr_Remove(p,a) (p)->lpVtbl->Remove(p,a)
#endif

/* IACList interface */
#define INTERFACE IACList
DECLARE_INTERFACE_(IACList,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface) (THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IACList methods ***/
    STDMETHOD(Expand)(THIS_ LPCOLESTR str) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IACList_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IACList_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IACList_Release(p)             (p)->lpVtbl->Release(p)
/*** IACList methods ***/
#define IACList_Expand(p,a)             (p)->lpVtbl->Expand(p,a)
#endif

/* IProgressDialog interface */
#define PROGDLG_NORMAL           0x00000000
#define PROGDLG_MODAL            0x00000001
#define PROGDLG_AUTOTIME         0x00000002
#define PROGDLG_NOTIME           0x00000004
#define PROGDLG_NOMINIMIZE       0x00000008
#define PROGDLG_NOPROGRESSBAR    0x00000010
#define PROGDLG_MARQUEEPROGRESS  0x00000020
#define PROGDLG_NOCANCEL         0x00000040

#define PDTIMER_RESET            0x00000001
#define PDTIMER_PAUSE            0x00000002
#define PDTIMER_RESUME           0x00000003

#define INTERFACE IProgressDialog
DECLARE_INTERFACE_(IProgressDialog,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface) (THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IProgressDialog methods ***/
    STDMETHOD(StartProgressDialog)(THIS_ HWND hwndParent, IUnknown *punkEnableModeless, DWORD dwFlags, LPCVOID reserved) PURE;
    STDMETHOD(StopProgressDialog)(THIS) PURE;
    STDMETHOD(SetTitle)(THIS_ LPCWSTR pwzTitle) PURE;
    STDMETHOD(SetAnimation)(THIS_ HINSTANCE hInstance, UINT uiResourceId) PURE;
    STDMETHOD_(BOOL,HasUserCancelled)(THIS) PURE;
    STDMETHOD(SetProgress)(THIS_ DWORD dwCompleted, DWORD dwTotal) PURE;
    STDMETHOD(SetProgress64)(THIS_ ULONGLONG ullCompleted, ULONGLONG ullTotal) PURE;
    STDMETHOD(SetLine)(THIS_ DWORD dwLineNum, LPCWSTR pwzString, BOOL bPath, LPCVOID reserved) PURE;
    STDMETHOD(SetCancelMsg)(THIS_ LPCWSTR pwzCancelMsg, LPCVOID reserved) PURE;
    STDMETHOD(Timer)(THIS_ DWORD dwTimerAction, LPCVOID reserved) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IProgressDialog_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IProgressDialog_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IProgressDialog_Release(p)             (p)->lpVtbl->Release(p)
/*** IProgressDialog methods ***/
#define IProgressDialog_StartProgressDialog(p,a,b,c,d)    (p)->lpVtbl->StartProgressDialog(p,a,b,c,d)
#define IProgressDialog_StopProgressDialog(p)             (p)->lpVtbl->StopProgressDialog(p)
#define IProgressDialog_SetTitle(p,a)                     (p)->lpVtbl->SetTitle(p,a)
#define IProgressDialog_SetAnimation(p,a,b)               (p)->lpVtbl->SetAnimation(p,a,b)
#define IProgressDialog_HasUserCancelled(p)               (p)->lpVtbl->HasUserCancelled(p)
#define IProgressDialog_SetProgress(p,a,b)                (p)->lpVtbl->SetProgress(p,a,b)
#define IProgressDialog_SetProgress64(p,a,b)              (p)->lpVtbl->SetProgress64(p,a,b)
#define IProgressDialog_SetLine(p,a,b,c,d)                (p)->lpVtbl->SetLine(p,a,b,c,d)
#define IProgressDialog_SetCancelMsg(p,a,b)               (p)->lpVtbl->SetCancelMsg(p,a,b)
#define IProgressDialog_Timer(p,a,b)                      (p)->lpVtbl->Timer(p,a,b)
#endif


/****************************************************************************
* SHAddToRecentDocs API
*/
#define SHARD_PIDL      0x00000001L
#define SHARD_PATHA     0x00000002L
#define SHARD_PATHW     0x00000003L
#define SHARD_PATH WINELIB_NAME_AW(SHARD_PATH)

void WINAPI SHAddToRecentDocs(UINT,LPCVOID);

/****************************************************************************
 * SHBrowseForFolder API
 */
typedef INT (CALLBACK *BFFCALLBACK)(HWND,UINT,LPARAM,LPARAM);

#include <pshpack8.h>

typedef struct tagBROWSEINFOA {
    HWND        hwndOwner;
    LPCITEMIDLIST pidlRoot;
    LPSTR         pszDisplayName;
    LPCSTR        lpszTitle;
    UINT        ulFlags;
    BFFCALLBACK   lpfn;
    LPARAM        lParam;
    INT         iImage;
} BROWSEINFOA, *PBROWSEINFOA, *LPBROWSEINFOA;

typedef struct tagBROWSEINFOW {
    HWND        hwndOwner;
    LPCITEMIDLIST pidlRoot;
    LPWSTR        pszDisplayName;
    LPCWSTR       lpszTitle;
    UINT        ulFlags;
    BFFCALLBACK   lpfn;
    LPARAM        lParam;
    INT         iImage;
} BROWSEINFOW, *PBROWSEINFOW, *LPBROWSEINFOW;

#define BROWSEINFO   WINELIB_NAME_AW(BROWSEINFO)
#define PBROWSEINFO  WINELIB_NAME_AW(PBROWSEINFO)
#define LPBROWSEINFO WINELIB_NAME_AW(LPBROWSEINFO)

#include <poppack.h>

/* Browsing for directory. */
#define BIF_RETURNONLYFSDIRS   0x0001
#define BIF_DONTGOBELOWDOMAIN  0x0002
#define BIF_STATUSTEXT         0x0004
#define BIF_RETURNFSANCESTORS  0x0008
#define BIF_EDITBOX            0x0010
#define BIF_VALIDATE           0x0020
#define BIF_NEWDIALOGSTYLE     0x0040
#define BIF_USENEWUI           (BIF_NEWDIALOGSTYLE | BIF_EDITBOX)
#define BIF_BROWSEINCLUDEURLS  0x0080
#define BIF_UAHINT             0x0100
#define BIF_NONEWFOLDERBUTTON  0x0200
#define BIF_NOTRANSLATETARGETS 0x0400

#define BIF_BROWSEFORCOMPUTER  0x1000
#define BIF_BROWSEFORPRINTER   0x2000
#define BIF_BROWSEINCLUDEFILES 0x4000

/* message from browser */
#define BFFM_INITIALIZED        1
#define BFFM_SELCHANGED         2
#define BFFM_VALIDATEFAILEDA    3
#define BFFM_VALIDATEFAILEDW    4
#define BFFM_IUNKNOWN           5

/* messages to browser */
#define BFFM_SETSTATUSTEXTA     (WM_USER+100)
#define BFFM_ENABLEOK           (WM_USER+101)
#define BFFM_SETSELECTIONA      (WM_USER+102)
#define BFFM_SETSELECTIONW      (WM_USER+103)
#define BFFM_SETSTATUSTEXTW     (WM_USER+104)
#define BFFM_SETOKTEXT          (WM_USER+105)
#define BFFM_SETEXPANDED        (WM_USER+106)

LPITEMIDLIST WINAPI SHBrowseForFolderA(LPBROWSEINFOA lpbi);
LPITEMIDLIST WINAPI SHBrowseForFolderW(LPBROWSEINFOW lpbi);
#define SHBrowseForFolder	 WINELIB_NAME_AW(SHBrowseForFolder)
#define BFFM_SETSTATUSTEXT  WINELIB_NAME_AW(BFFM_SETSTATUSTEXT)
#define BFFM_SETSELECTION   WINELIB_NAME_AW(BFFM_SETSELECTION)
#define BFFM_VALIDATEFAILED WINELIB_NAME_AW(BFFM_VALIDATEFAILED)

/**********************************************************************
 * SHCreateShellFolderViewEx API
 */

typedef HRESULT (CALLBACK *LPFNVIEWCALLBACK)(
	IShellView* dwUser,
	IShellFolder* pshf,
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam);

typedef struct _CSFV
{
  UINT             cbSize;
  IShellFolder*    pshf;
  IShellView*      psvOuter;
  LPCITEMIDLIST    pidl;
  LONG             lEvents;
  LPFNVIEWCALLBACK pfnCallback;
  FOLDERVIEWMODE   fvm;
} CSFV, *LPCSFV;

HRESULT WINAPI SHCreateShellFolderViewEx(LPCSFV pshfvi, IShellView **ppshv);

/* SHCreateShellFolderViewEx callback messages */
#define SFVM_MERGEMENU                 1
#define SFVM_INVOKECOMMAND             2
#define SFVM_GETHELPTEXT               3
#define SFVM_GETTOOLTIPTEXT            4
#define SFVM_GETBUTTONINFO             5
#define SFVM_GETBUTTONS                6
#define SFVM_INITMENUPOPUP             7
#define SFVM_SELECTIONCHANGED          8 /* undocumented */
#define SFVM_DRAWMENUITEM              9 /* undocumented */
#define SFVM_MEASUREMENUITEM          10 /* undocumented */
#define SFVM_EXITMENULOOP             11 /* undocumented */
#define SFVM_VIEWRELEASE              12 /* undocumented */
#define SFVM_GETNAMELENGTH            13 /* undocumented */
#define SFVM_FSNOTIFY                 14
#define SFVM_WINDOWCREATED            15
#define SFVM_WINDOWCLOSING            16 /* undocumented */
#define SFVM_LISTREFRESHED            17 /* undocumented */
#define SFVM_WINDOWFOCUSED            18 /* undocumented */
#define SFVM_REGISTERCOPYHOOK         20 /* undocumented */
#define SFVM_COPYHOOKCALLBACK         21 /* undocumented */
#define SFVM_GETDETAILSOF             23
#define SFVM_COLUMNCLICK              24
#define SFVM_QUERYFSNOTIFY            25
#define SFVM_DEFITEMCOUNT             26
#define SFVM_DEFVIEWMODE              27
#define SFVM_UNMERGEFROMMENU          28
#define SFVM_ADDINGOBJECT             29 /* undocumented */
#define SFVM_REMOVINGOBJECT           30 /* undocumented */
#define SFVM_UPDATESTATUSBAR          31
#define SFVM_BACKGROUNDENUM           32
#define SFVM_GETCOMMANDDIR            33 /* undocumented */
#define SFVM_GETCOLUMNSTREAM          34 /* undocumented */
#define SFVM_CANSELECTALL             35 /* undocumented */
#define SFVM_DIDDRAGDROP              36
#define SFVM_ISSTRICTREFRESH          37 /* undocumented */
#define SFVM_ISCHILDOBJECT            38 /* undocumented */
#define SFVM_SETISFV                  39
#define SFVM_GETEXTVIEWS              40 /* undocumented */
#define SFVM_THISIDLIST               41
#define SFVM_ADDPROPERTYPAGES         47
#define SFVM_BACKGROUNDENUMDONE       48
#define SFVM_GETNOTIFY                49
#define SFVM_GETSORTDEFAULTS          53
#define SFVM_SIZE                     57
#define SFVM_GETZONE                  58
#define SFVM_GETPANE                  59
#define SFVM_GETHELPTOPIC             63
#define SFVM_GETANIMATION             68
#define SFVM_GET_CUSTOMVIEWINFO       77 /* undocumented */
#define SFVM_ENUMERATEDITEMS          79 /* undocumented */
#define SFVM_GET_VIEW_DATA            80 /* undocumented */
#define SFVM_GET_WEBVIEW_LAYOUT       82 /* undocumented */
#define SFVM_GET_WEBVIEW_CONTENT      83 /* undocumented */
#define SFVM_GET_WEBVIEW_TASKS        84 /* undocumented */
#define SFVM_GET_WEBVIEW_THEME        86 /* undocumented */
#define SFVM_GETDEFERREDVIEWSETTINGS  92 /* undocumented */

/* Types and definitions for the SFM_* parameters */
#include <pshpack8.h>

#define QCMINFO_PLACE_BEFORE          0
#define QCMINFO_PLACE_AFTER           1
typedef struct _QCMINFO_IDMAP_PLACEMENT
{
    UINT id;
    UINT fFlags;
} QCMINFO_IDMAP_PLACEMENT;

typedef struct _QCMINFO_IDMAP
{
    UINT nMaxIds;
    QCMINFO_IDMAP_PLACEMENT pIdList[1];
} QCMINFO_IDMAP;

typedef struct _QCMINFO
{
    HMENU hmenu;
    UINT indexMenu;
    UINT idCmdFirst;
    UINT idCmdLast;
    QCMINFO_IDMAP const* pIdMap;
} QCMINFO, *LPQCMINFO;

#define TBIF_DEFAULT           0x00000000
#define TBIF_APPEND            0x00000000
#define TBIF_PREPEND           0x00000001
#define TBIF_REPLACE           0x00000002
#define TBIF_INTERNETBAR       0x00010000
#define TBIF_STANDARDTOOLBAR   0x00020000
#define TBIF_NOTOOLBAR         0x00030000

typedef struct _TBINFO
{
    UINT cbuttons;
    UINT uFlags;
} TBINFO, *LPTBINFO;

#include <poppack.h>

/****************************************************************************
*	SHShellFolderView_Message API
*/

LRESULT WINAPI SHShellFolderView_Message(
	HWND hwndCabinet,
	UINT uMessage,
	LPARAM lParam);

/* SHShellFolderView_Message messages */
#define SFVM_REARRANGE          0x0001
#define SFVM_GETARRANGECOLUMN   0x0002 /* undocumented */
#define SFVM_ADDOBJECT          0x0003
#define SFVM_GETITEMCOUNT       0x0004 /* undocumented */
#define SFVM_GETITEMPIDL        0x0005 /* undocumented */
#define SFVM_REMOVEOBJECT       0x0006
#define SFVM_UPDATEOBJECT       0x0007
#define SFVM_SETREDRAW          0x0008 /* undocumented */
#define SFVM_GETSELECTEDOBJECTS 0x0009
#define SFVM_ISDROPONSOURCE     0x000A /* undocumented */
#define SFVM_MOVEICONS          0x000B /* undocumented */
#define SFVM_GETDRAGPOINT       0x000C /* undocumented */
#define SFVM_GETDROPPOINT       0x000D /* undocumented */
#define SFVM_SETITEMPOS         0x000E
#define SFVM_ISDROPONBACKGROUND 0x000F /* undocumented */
#define SFVM_SETCLIPBOARD       0x0010
#define SFVM_TOGGLEAUTOARRANGE  0x0011 /* undocumented */
#define SFVM_LINEUPICONS        0x0012 /* undocumented */
#define SFVM_GETAUTOARRANGE     0x0013 /* undocumented */
#define SFVM_GETSELECTEDCOUNT   0x0014 /* undocumented */
#define SFVM_GETITEMSPACING     0x0015 /* undocumented */
#define SFVM_REFRESHOBJECT      0x0016 /* undocumented */
#define SFVM_SETPOINTS          0x0017

/****************************************************************************
*	SHGetDataFromIDList API
*/
#define SHGDFIL_FINDDATA        1
#define SHGDFIL_NETRESOURCE     2
#define SHGDFIL_DESCRIPTIONID   3

#define SHDID_ROOT_REGITEM          1
#define SHDID_FS_FILE               2
#define SHDID_FS_DIRECTORY          3
#define SHDID_FS_OTHER              4
#define SHDID_COMPUTER_DRIVE35      5
#define SHDID_COMPUTER_DRIVE525     6
#define SHDID_COMPUTER_REMOVABLE    7
#define SHDID_COMPUTER_FIXED        8
#define SHDID_COMPUTER_NETDRIVE     9
#define SHDID_COMPUTER_CDROM        10
#define SHDID_COMPUTER_RAMDISK      11
#define SHDID_COMPUTER_OTHER        12
#define SHDID_NET_DOMAIN            13
#define SHDID_NET_SERVER            14
#define SHDID_NET_SHARE             15
#define SHDID_NET_RESTOFNET         16
#define SHDID_NET_OTHER             17
#define SHDID_COMPUTER_IMAGING      18
#define SHDID_COMPUTER_AUDIO        19
#define SHDID_COMPUTER_SHAREDDOCS   20

#include <pshpack8.h>

typedef struct _SHDESCRIPTIONID
{   DWORD   dwDescriptionId;
    CLSID   clsid;
} SHDESCRIPTIONID, *LPSHDESCRIPTIONID;

#include <poppack.h>

HRESULT WINAPI SHGetDataFromIDListA(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, int nFormat, LPVOID pv, int cb);
HRESULT WINAPI SHGetDataFromIDListW(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, int nFormat, LPVOID pv, int cb);
#define  SHGetDataFromIDList WINELIB_NAME_AW(SHGetDataFromIDList)

BOOL WINAPI SHGetSpecialFolderPathA (HWND hwndOwner, LPSTR szPath, int nFolder, BOOL bCreate);
BOOL WINAPI SHGetSpecialFolderPathW (HWND hwndOwner, LPWSTR szPath, int nFolder, BOOL bCreate);
#define  SHGetSpecialFolderPath WINELIB_NAME_AW(SHGetSpecialFolderPath)

HRESULT WINAPI SHGetMalloc(LPMALLOC *lpmal) ;

/**********************************************************************
 * SHGetSetSettings ()
 */

typedef struct
{
    BOOL fShowAllObjects : 1;
    BOOL fShowExtensions : 1;
    BOOL fNoConfirmRecycle : 1;

    BOOL fShowSysFiles : 1;
    BOOL fShowCompColor : 1;
    BOOL fDoubleClickInWebView : 1;
    BOOL fDesktopHTML : 1;
    BOOL fWin95Classic : 1;
    BOOL fDontPrettyPath : 1;
    BOOL fShowAttribCol : 1;
    BOOL fMapNetDrvBtn : 1;
    BOOL fShowInfoTip : 1;
    BOOL fHideIcons : 1;
    BOOL fWebView : 1;
    BOOL fFilter : 1;
    BOOL fShowSuperHidden : 1;
    BOOL fNoNetCrawling : 1;

    UINT :15; /* Required for proper binary layout with gcc */
    DWORD dwWin95Unused;
    UINT  uWin95Unused;
    LONG   lParamSort;
    int    iSortDirection;
    UINT   version;
    UINT uNotUsed;
    BOOL fSepProcess: 1;
    BOOL fStartPanelOn: 1;
    BOOL fShowStartPage: 1;
    UINT fSpareFlags : 13;
    UINT :15; /* Required for proper binary layout with gcc */
} SHELLSTATE, *LPSHELLSTATE;

/**********************************************************************
 * SHGetSettings ()
 */
typedef struct
{	BOOL fShowAllObjects : 1;
	BOOL fShowExtensions : 1;
	BOOL fNoConfirmRecycle : 1;
	BOOL fShowSysFiles : 1;

	BOOL fShowCompColor : 1;
	BOOL fDoubleClickInWebView : 1;
	BOOL fDesktopHTML : 1;
	BOOL fWin95Classic : 1;

	BOOL fDontPrettyPath : 1;
	BOOL fShowAttribCol : 1;
	BOOL fMapNetDrvBtn : 1;
	BOOL fShowInfoTip : 1;

	BOOL fHideIcons : 1;
	UINT fRestFlags : 3;
	UINT :15; /* Required for proper binary layout with gcc */
} SHELLFLAGSTATE, * LPSHELLFLAGSTATE;

VOID WINAPI SHGetSettings(LPSHELLFLAGSTATE lpsfs, DWORD dwMask);

#define SSF_SHOWALLOBJECTS		0x0001
#define SSF_SHOWEXTENSIONS		0x0002
#define SSF_SHOWCOMPCOLOR		0x0008
#define SSF_SHOWSYSFILES		0x0020
#define SSF_DOUBLECLICKINWEBVIEW	0x0080
#define SSF_SHOWATTRIBCOL		0x0100
#define SSF_DESKTOPHTML			0x0200
#define SSF_WIN95CLASSIC		0x0400
#define SSF_DONTPRETTYPATH		0x0800
#define SSF_SHOWINFOTIP			0x2000
#define SSF_MAPNETDRVBUTTON		0x1000
#define SSF_NOCONFIRMRECYCLE		0x8000
#define SSF_HIDEICONS			0x4000

/****************************************************************************
* SHRestricted API
*/
typedef enum RESTRICTIONS
{
	REST_NONE			= 0x00000000,
	REST_NORUN			= 0x00000001,
	REST_NOCLOSE			= 0x00000002,
	REST_NOSAVESET			= 0x00000004,
	REST_NOFILEMENU			= 0x00000008,
	REST_NOSETFOLDERS		= 0x00000010,
	REST_NOSETTASKBAR		= 0x00000020,
	REST_NODESKTOP			= 0x00000040,
	REST_NOFIND			= 0x00000080,
	REST_NODRIVES			= 0x00000100,
	REST_NODRIVEAUTORUN		= 0x00000200,
	REST_NODRIVETYPEAUTORUN		= 0x00000400,
	REST_NONETHOOD			= 0x00000800,
	REST_STARTBANNER		= 0x00001000,
	REST_RESTRICTRUN		= 0x00002000,
	REST_NOPRINTERTABS		= 0x00004000,
	REST_NOPRINTERDELETE		= 0x00008000,
	REST_NOPRINTERADD		= 0x00010000,
	REST_NOSTARTMENUSUBFOLDERS	= 0x00020000,
	REST_MYDOCSONNET		= 0x00040000,
	REST_NOEXITTODOS		= 0x00080000,
	REST_ENFORCESHELLEXTSECURITY	= 0x00100000,
	REST_LINKRESOLVEIGNORELINKINFO	= 0x00200000,
	REST_NOCOMMONGROUPS		= 0x00400000,
	REST_SEPARATEDESKTOPPROCESS	= 0x00800000,
	REST_NOWEB			= 0x01000000,
	REST_NOTRAYCONTEXTMENU		= 0x02000000,
	REST_NOVIEWCONTEXTMENU		= 0x04000000,
	REST_NONETCONNECTDISCONNECT	= 0x08000000,
	REST_STARTMENULOGOFF		= 0x10000000,
	REST_NOSETTINGSASSIST		= 0x20000000,
	REST_NOINTERNETICON		= 0x40000001,
	REST_NORECENTDOCSHISTORY,
	REST_NORECENTDOCSMENU,
	REST_NOACTIVEDESKTOP,
	REST_NOACTIVEDESKTOPCHANGES,
	REST_NOFAVORITESMENU,
	REST_CLEARRECENTDOCSONEXIT,
	REST_CLASSICSHELL,
	REST_NOCUSTOMIZEWEBVIEW,

	REST_NOHTMLWALLPAPER		= 0x40000010,
	REST_NOCHANGINGWALLPAPER,
	REST_NODESKCOMP,
	REST_NOADDDESKCOMP,
	REST_NODELDESKCOMP,
	REST_NOCLOSEDESKCOMP,
	REST_NOCLOSE_DRAGDROPBAND,
	REST_NOMOVINGBAND,
	REST_NOEDITDESKCOMP,
	REST_NORESOLVESEARCH,
	REST_NORESOLVETRACK,
	REST_FORCECOPYACLWITHFILE,
	REST_NOLOGO3CHANNELNOTIFY,
	REST_NOFORGETSOFTWAREUPDATE,
	REST_NOSETACTIVEDESKTOP,
	REST_NOUPDATEWINDOWS,
	REST_NOCHANGESTARMENU,		/* 0x40000020 */
	REST_NOFOLDEROPTIONS,
	REST_HASFINDCOMPUTERS,
	REST_INTELLIMENUS,
	REST_RUNDLGMEMCHECKBOX,
	REST_ARP_ShowPostSetup,
	REST_NOCSC,
	REST_NOCONTROLPANEL,
	REST_ENUMWORKGROUP,
	REST_ARP_NOARP,
	REST_ARP_NOREMOVEPAGE,
	REST_ARP_NOADDPAGE,
	REST_ARP_NOWINSETUPPAGE,
	REST_GREYMSIADS,
	REST_NOCHANGEMAPPEDDRIVELABEL,
	REST_NOCHANGEMAPPEDDRIVECOMMENT,
	REST_MaxRecentDocs,		/* 0x40000030 */
	REST_NONETWORKCONNECTIONS,
	REST_FORCESTARTMENULOGOFF,
	REST_NOWEBVIEW,
	REST_NOCUSTOMIZETHISFOLDER,
	REST_NOENCRYPTION,

	REST_ALLOWFRENCHENCRYPTION,	/* not documented */

	REST_DONTSHOWSUPERHIDDEN,
	REST_NOSHELLSEARCHBUTTON,
	REST_NOHARDWARETAB,
	REST_NORUNASINSTALLPROMPT,
	REST_PROMPTRUNASINSTALLNETPATH,
	REST_NOMANAGEMYCOMPUTERVERB,
	REST_NORECENTDOCSNETHOOD,
	REST_DISALLOWRUN,
	REST_NOWELCOMESCREEN,
	REST_RESTRICTCPL,		/* 0x40000040 */
	REST_DISALLOWCPL,
	REST_NOSMBALLOONTIP,
	REST_NOSMHELP,
	REST_NOWINKEYS,
	REST_NOENCRYPTONMOVE,
	REST_NOLOCALMACHINERUN,
	REST_NOCURRENTUSERRUN,
	REST_NOLOCALMACHINERUNONCE,
	REST_NOCURRENTUSERRUNONCE,
	REST_FORCEACTIVEDESKTOPON,
	REST_NOCOMPUTERSNEARME,
	REST_NOVIEWONDRIVE,
	REST_NONETCRAWL,
	REST_NOSHAREDDOCUMENTS,
	REST_NOSMMYDOCS,
	REST_NOSMMYPICS,		/* 0x40000050 */
	REST_ALLOWBITBUCKDRIVES,
	REST_NONLEGACYSHELLMODE,
	REST_NOCONTROLPANELBARRICADE,
	REST_NOSTARTPAGE,
	REST_NOAUTOTRAYNOTIFY,
	REST_NOTASKGROUPING,
	REST_NOCDBURNING,
	REST_MYCOMPNOPROP,
	REST_MYDOCSNOPROP,
	REST_NOSTARTPANEL,
	REST_NODISPLAYAPPEARANCEPAGE,
	REST_NOTHEMESTAB,
	REST_NOVISUALSTYLECHOICE,
	REST_NOSIZECHOICE,
	REST_NOCOLORCHOICE,
	REST_SETVISUALSTYLE,		/* 0x40000060 */
	REST_STARTRUNNOHOMEPATH,
	REST_NOUSERNAMEINSTARTPANEL,
	REST_NOMYCOMPUTERICON,
	REST_NOSMNETWORKPLACES,
	REST_NOSMPINNEDLIST,
	REST_NOSMMYMUSIC,
	REST_NOSMEJECTPC,
	REST_NOSMMOREPROGRAMS,
	REST_NOSMMFUPROGRAMS,
	REST_NOTRAYITEMSDISPLAY,
	REST_NOTOOLBARSONTASKBAR,
	/* 0x4000006C
	   0x4000006D
	   0x4000006E */
	REST_NOSMCONFIGUREPROGRAMS	= 0x4000006F,
	REST_HIDECLOCK,			/* 0x40000070 */
	REST_NOLOWDISKSPACECHECKS,
	REST_NOENTIRENETWORK,
	REST_NODESKTOPCLEANUP,
	REST_BITBUCKNUKEONDELETE,
	REST_BITBUCKCONFIRMDELETE,
	REST_BITBUCKNOPROP,
	REST_NODISPBACKGROUND,
	REST_NODISPSCREENSAVEPG,
	REST_NODISPSETTINGSPG,
	REST_NODISPSCREENSAVEPREVIEW,
	REST_NODISPLAYCPL,
	REST_HIDERUNASVERB,
	REST_NOTHUMBNAILCACHE,
	REST_NOSTRCMPLOGICAL,
	REST_NOPUBLISHWIZARD,
	REST_NOONLINEPRINTSWIZARD,	/* 0x40000080 */
	REST_NOWEBSERVICES,
	REST_ALLOWUNHASHEDWEBVIEW,
	REST_ALLOWLEGACYWEBVIEW,
	REST_REVERTWEBVIEWSECURITY,

	REST_INHERITCONSOLEHANDLES	= 0x40000086,

	REST_NODISCONNECT		= 0x41000001,
	REST_NOSECURITY,
	REST_NOFILEASSOCIATE,		/* 0x41000003 */
} RESTRICTIONS;

DWORD WINAPI SHRestricted(RESTRICTIONS rest);

/****************************************************************************
* SHChangeNotify API
*/
typedef struct _SHChangeNotifyEntry
{
    LPCITEMIDLIST pidl;
    BOOL   fRecursive;
} SHChangeNotifyEntry;

#define SHCNE_RENAMEITEM	0x00000001
#define SHCNE_CREATE		0x00000002
#define SHCNE_DELETE		0x00000004
#define SHCNE_MKDIR		0x00000008
#define SHCNE_RMDIR		0x00000010
#define SHCNE_MEDIAINSERTED	0x00000020
#define SHCNE_MEDIAREMOVED	0x00000040
#define SHCNE_DRIVEREMOVED	0x00000080
#define SHCNE_DRIVEADD		0x00000100
#define SHCNE_NETSHARE		0x00000200
#define SHCNE_NETUNSHARE	0x00000400
#define SHCNE_ATTRIBUTES	0x00000800
#define SHCNE_UPDATEDIR		0x00001000
#define SHCNE_UPDATEITEM	0x00002000
#define SHCNE_SERVERDISCONNECT	0x00004000
#define SHCNE_UPDATEIMAGE	0x00008000
#define SHCNE_DRIVEADDGUI	0x00010000
#define SHCNE_RENAMEFOLDER	0x00020000
#define SHCNE_FREESPACE		0x00040000

#define SHCNE_EXTENDED_EVENT	0x04000000
#define SHCNE_ASSOCCHANGED	0x08000000
#define SHCNE_DISKEVENTS	0x0002381F
#define SHCNE_GLOBALEVENTS	0x0C0581E0
#define SHCNE_ALLEVENTS		0x7FFFFFFF
#define SHCNE_INTERRUPT		0x80000000

#define SHCNEE_ORDERCHANGED	0x0002L
#define SHCNEE_MSI_CHANGE	0x0004L
#define SHCNEE_MSI_UNINSTALL	0x0005L

#define SHCNF_IDLIST		0x0000
#define SHCNF_PATHA		0x0001
#define SHCNF_PRINTERA		0x0002
#define SHCNF_DWORD		0x0003
#define SHCNF_PATHW		0x0005
#define SHCNF_PRINTERW		0x0006
#define SHCNF_TYPE		0x00FF
#define SHCNF_FLUSH		0x1000
#define SHCNF_FLUSHNOWAIT	0x2000

#define SHCNF_PATH              WINELIB_NAME_AW(SHCNF_PATH)
#define SHCNF_PRINTER           WINELIB_NAME_AW(SHCNF_PRINTER)

void WINAPI SHChangeNotify(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2);

typedef enum {
    SLDF_HAS_ID_LIST = 0x00000001,
    SLDF_HAS_LINK_INFO = 0x00000002,
    SLDF_HAS_NAME = 0x00000004,
    SLDF_HAS_RELPATH = 0x00000008,
    SLDF_HAS_WORKINGDIR = 0x00000010,
    SLDF_HAS_ARGS = 0x00000020,
    SLDF_HAS_ICONLOCATION = 0x00000040,
    SLDF_UNICODE = 0x00000080,
    SLDF_FORCE_NO_LINKINFO = 0x00000100,
    SLDF_HAS_EXP_SZ = 0x00000200,
    SLDF_RUN_IN_SEPARATE = 0x00000400,
    SLDF_HAS_LOGO3ID = 0x00000800,
    SLDF_HAS_DARWINID = 0x00001000,
    SLDF_RUNAS_USER = 0x00002000,
    SLDF_HAS_EXP_ICON_SZ = 0x00004000,
    SLDF_NO_PIDL_ALIAS = 0x00008000,
    SLDF_FORCE_UNCNAME = 0x00010000,
    SLDF_RUN_WITH_SHIMLAYER = 0x00020000,
    SLDF_FORCE_NO_LINKTRACK = 0x00040000,
    SLDF_ENABLE_TARGET_METADATA = 0x00080000,
    SLDF_DISABLE_KNOWNFOLDER_RELATIVE_TRACKING = 0x00200000,
    SLDF_VALID = 0x003ff7ff,
    SLDF_RESERVED = 0x80000000,
} SHELL_LINK_DATA_FLAGS;

typedef struct tagDATABLOCKHEADER
{
    DWORD cbSize;
    DWORD dwSignature;
} DATABLOCK_HEADER, *LPDATABLOCK_HEADER, *LPDBLIST;

typedef struct {
    DATABLOCK_HEADER dbh;
    CHAR szDarwinID[MAX_PATH];
    WCHAR szwDarwinID[MAX_PATH];
} EXP_DARWIN_LINK, *LPEXP_DARWIN_LINK;

typedef struct {
    DWORD cbSize;
    DWORD cbSignature;
    CHAR szTarget[MAX_PATH];
    WCHAR szwTarget[MAX_PATH];
} EXP_SZ_LINK, *LPEXP_SZ_LINK;

typedef struct {
    DWORD cbSize;
    DWORD dwSignature;
    DWORD idSpecialFolder;
    DWORD cbOffset;
} EXP_SPECIAL_FOLDER, *LPEXP_SPECIAL_FOLDER;

typedef struct {
    DWORD cbSize;
    DWORD dwSignature;
    BYTE abPropertyStorage[1];
} EXP_PROPERTYSTORAGE;

#define EXP_SZ_LINK_SIG         0xa0000001
#define NT_CONSOLE_PROPS_SIG    0xa0000002
#define NT_FE_CONSOLE_PROPS_SIG 0xa0000004
#define EXP_SPECIAL_FOLDER_SIG  0xa0000005
#define EXP_DARWIN_ID_SIG       0xa0000006
#define EXP_SZ_ICON_SIG         0xa0000007
#define EXP_LOGO3_ID_SIG        EXP_SZ_ICON_SIG /* Old SDKs only */
#define EXP_PROPERTYSTORAGE_SIG 0xa0000009

typedef struct _SHChangeDWORDAsIDList {
    USHORT   cb;
    DWORD    dwItem1;
    DWORD    dwItem2;
    USHORT   cbZero;
} SHChangeDWORDAsIDList, *LPSHChangeDWORDAsIDList;

typedef struct _SHChangeProductKeyAsIDList {
    USHORT cb;
    WCHAR wszProductKey[39];
    USHORT cbZero;
} SHChangeProductKeyAsIDList, *LPSHChangeProductKeyAsIDList;

ULONG WINAPI SHChangeNotifyRegister(HWND hwnd, int fSources, LONG fEvents, UINT wMsg,
                                    int cEntries, SHChangeNotifyEntry *pshcne);
BOOL WINAPI SHChangeNotifyDeregister(ULONG ulID);
HANDLE WINAPI SHChangeNotification_Lock(HANDLE hChangeNotification, DWORD dwProcessId,
                                        LPITEMIDLIST **pppidl, LONG *plEvent);
BOOL WINAPI SHChangeNotification_Unlock(HANDLE hLock);

HRESULT WINAPI SHGetRealIDL(IShellFolder *psf, LPCITEMIDLIST pidlSimple, LPITEMIDLIST * ppidlReal);

/****************************************************************************
* SHCreateDirectory API
*/
DWORD WINAPI SHCreateDirectory(HWND, LPCVOID);
int WINAPI SHCreateDirectoryExA(HWND, LPCSTR, LPSECURITY_ATTRIBUTES);
int WINAPI SHCreateDirectoryExW(HWND, LPCWSTR, LPSECURITY_ATTRIBUTES);

/****************************************************************************
* SHGetSpecialFolderLocation API
*/
HRESULT WINAPI SHGetSpecialFolderLocation(HWND hwndOwner, int nFolder, LPITEMIDLIST * ppidl);
HRESULT WINAPI SHGetFolderLocation(HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwReserved, LPITEMIDLIST *ppidl);

/****************************************************************************
* SHGetFolderPath API
*/
typedef enum {
    SHGFP_TYPE_CURRENT = 0,
    SHGFP_TYPE_DEFAULT = 1
} SHGFP_TYPE;

HRESULT WINAPI SHGetFolderPathA(HWND hwnd, int nFolder, HANDLE hToken, DWORD dwFlags, LPSTR pszPath);
HRESULT WINAPI SHGetFolderPathW(HWND hwnd, int nFolder, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath);
#define        SHGetFolderPath WINELIB_NAME_AW(SHGetFolderPath)

#define CSIDL_DESKTOP		0x0000
#define CSIDL_INTERNET		0x0001
#define CSIDL_PROGRAMS		0x0002
#define CSIDL_CONTROLS		0x0003
#define CSIDL_PRINTERS		0x0004
#define CSIDL_PERSONAL		0x0005
#define CSIDL_FAVORITES		0x0006
#define CSIDL_STARTUP		0x0007
#define CSIDL_RECENT		0x0008
#define CSIDL_SENDTO		0x0009
#define CSIDL_BITBUCKET		0x000a
#define CSIDL_STARTMENU		0x000b
#define CSIDL_MYDOCUMENTS	CSIDL_PERSONAL
#define CSIDL_MYMUSIC		0x000d
#define CSIDL_MYVIDEO		0x000e
#define CSIDL_DESKTOPDIRECTORY	0x0010
#define CSIDL_DRIVES		0x0011
#define CSIDL_NETWORK		0x0012
#define CSIDL_NETHOOD		0x0013
#define CSIDL_FONTS		0x0014
#define CSIDL_TEMPLATES		0x0015
#define CSIDL_COMMON_STARTMENU	0x0016
#define CSIDL_COMMON_PROGRAMS	0X0017
#define CSIDL_COMMON_STARTUP	0x0018
#define CSIDL_COMMON_DESKTOPDIRECTORY	0x0019
#define CSIDL_APPDATA		0x001a
#define CSIDL_PRINTHOOD		0x001b
#define CSIDL_LOCAL_APPDATA	0x001c
#define CSIDL_ALTSTARTUP	0x001d
#define CSIDL_COMMON_ALTSTARTUP	0x001e
#define CSIDL_COMMON_FAVORITES  0x001f
#define CSIDL_INTERNET_CACHE	0x0020
#define CSIDL_COOKIES		0x0021
#define CSIDL_HISTORY		0x0022
#define CSIDL_COMMON_APPDATA	0x0023
#define CSIDL_WINDOWS		0x0024
#define CSIDL_SYSTEM		0x0025
#define CSIDL_PROGRAM_FILES	0x0026
#define CSIDL_MYPICTURES	0x0027
#define CSIDL_PROFILE		0x0028
#define CSIDL_SYSTEMX86		0x0029
#define CSIDL_PROGRAM_FILESX86	0x002a
#define CSIDL_PROGRAM_FILES_COMMON	0x002b
#define CSIDL_PROGRAM_FILES_COMMONX86	0x002c
#define CSIDL_COMMON_TEMPLATES	0x002d
#define CSIDL_COMMON_DOCUMENTS	0x002e
#define CSIDL_COMMON_ADMINTOOLS	0x002f
#define CSIDL_ADMINTOOLS	0x0030
#define CSIDL_CONNECTIONS	0x0031
#define CSIDL_COMMON_MUSIC	0x0035
#define CSIDL_COMMON_PICTURES	0x0036
#define CSIDL_COMMON_VIDEO	0x0037
#define CSIDL_RESOURCES		0x0038
#define CSIDL_RESOURCES_LOCALIZED 0x0039
#define CSIDL_COMMON_OEM_LINKS	0x003a
#define CSIDL_CDBURN_AREA	0x003b
#define CSIDL_COMPUTERSNEARME	0x003d
#define CSIDL_PROFILES		0x003e
#define CSIDL_FOLDER_MASK	0x00ff
#define CSIDL_FLAG_PER_USER_INIT 0x0800
#define CSIDL_FLAG_NO_ALIAS	0x1000
#define CSIDL_FLAG_DONT_VERIFY	0x4000
#define CSIDL_FLAG_CREATE	0x8000

#define CSIDL_FLAG_MASK		0xff00

/****************************************************************************
 * SHGetDesktopFolder API
 */
HRESULT WINAPI SHGetDesktopFolder(IShellFolder * *);

/****************************************************************************
 * SHBindToParent API
 */
HRESULT WINAPI SHBindToParent(LPCITEMIDLIST pidl, REFIID riid, LPVOID *ppv, LPCITEMIDLIST *ppidlLast);

/****************************************************************************
* SHDefExtractIcon API
*/
HRESULT WINAPI SHDefExtractIconA(LPCSTR pszIconFile, int iIndex, UINT uFlags,
                                 HICON* phiconLarge, HICON* phiconSmall, UINT nIconSize);
HRESULT WINAPI SHDefExtractIconW(LPCWSTR pszIconFile, int iIndex, UINT uFlags,
                                 HICON* phiconLarge, HICON* phiconSmall, UINT nIconSize);
#define        SHDefExtractIcon WINELIB_NAME_AW(SHDefExtractIcon)

/*
 * DROPFILES for CF_HDROP and CF_PRINTERS
 */
typedef struct _DROPFILES
{
  DWORD pFiles;
  POINT pt;
  BOOL  fNC;
  BOOL  fWide;
} DROPFILES, *LPDROPFILES;

/*
 * Properties of a file in the clipboard
 */
typedef struct _FILEDESCRIPTORA {
    DWORD dwFlags;
    CLSID clsid;
    SIZEL sizel;
    POINTL pointl;
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    CHAR cFileName[MAX_PATH];
} FILEDESCRIPTORA, *LPFILEDESCRIPTORA;

typedef struct _FILEDESCRIPTORW {
    DWORD dwFlags;
    CLSID clsid;
    SIZEL sizel;
    POINTL pointl;
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    WCHAR cFileName[MAX_PATH];
} FILEDESCRIPTORW, *LPFILEDESCRIPTORW;

DECL_WINELIB_TYPE_AW(FILEDESCRIPTOR)
DECL_WINELIB_TYPE_AW(LPFILEDESCRIPTOR)

/*
 * CF_FILEGROUPDESCRIPTOR clipboard format
 */
typedef struct _FILEGROUPDESCRIPTORA {
    UINT cItems;
    FILEDESCRIPTORA fgd[1];
} FILEGROUPDESCRIPTORA, *LPFILEGROUPDESCRIPTORA;

typedef struct _FILEGROUPDESCRIPTORW {
    UINT cItems;
    FILEDESCRIPTORW fgd[1];
} FILEGROUPDESCRIPTORW, *LPFILEGROUPDESCRIPTORW;

DECL_WINELIB_TYPE_AW(FILEGROUPDESCRIPTOR)
DECL_WINELIB_TYPE_AW(LPFILEGROUPDESCRIPTOR)

/****************************************************************************
 * Cabinet functions
 */

typedef struct {
    WORD cLength;
    WORD nVersion;
    BOOL fFullPathTitle:1;
    BOOL fSaveLocalView:1;
    BOOL fNotShell:1;
    BOOL fSimpleDefault:1;
    BOOL fDontShowDescBar:1;
    BOOL fNewWindowMode:1;
    BOOL fShowCompColor:1;
    BOOL fDontPrettyNames:1;
    BOOL fAdminsCreateCommonGroups:1;
    UINT fUnusedFlags:7;
    UINT :15; /* Required for proper binary layout with gcc */
    UINT fMenuEnumFilter;
} CABINETSTATE, *LPCABINETSTATE;

#define CABINETSTATE_VERSION 2

BOOL WINAPI ReadCabinetState(CABINETSTATE *, int);
BOOL WINAPI WriteCabinetState(CABINETSTATE *);

/****************************************************************************
 * Path Manipulation Routines
 */

/* PathProcessCommand flags */
#define PPCF_ADDQUOTES        0x01
#define PPCF_INCLUDEARGS      0x02
#define PPCF_ADDARGUMENTS     0x03
#define PPCF_NODIRECTORIES    0x10
#define PPCF_DONTRESOLVE      0x20
#define PPCF_FORCEQUALIFY     0x40
#define PPCF_LONGESTPOSSIBLE  0x80

/* PathResolve flags */
#define PRF_VERIFYEXISTS         0x01
#define PRF_EXECUTABLE           0x02
#define PRF_TRYPROGRAMEXTENSIONS 0x03
#define PRF_FIRSTDIRDEF          0x04
#define PRF_DONTFINDLINK         0x08

VOID WINAPI PathGetShortPath(LPWSTR pszPath);
LONG WINAPI PathProcessCommand(LPCWSTR, LPWSTR, int, DWORD);
BOOL WINAPI PathYetAnotherMakeUniqueName(LPWSTR, LPCWSTR, LPCWSTR, LPCWSTR);

/****************************************************************************
 * Drag And Drop Routines
 */

/* DAD_AutoScroll sample structure */
#define NUM_POINTS 3
typedef struct
{
    int   iNextSample;
    DWORD dwLastScroll;
    BOOL  bFull;
    POINT pts[NUM_POINTS];
    DWORD dwTimes[NUM_POINTS];
} AUTO_SCROLL_DATA;

BOOL         WINAPI DAD_SetDragImage(HIMAGELIST,LPPOINT);
BOOL         WINAPI DAD_DragEnterEx(HWND,POINT);
BOOL         WINAPI DAD_DragEnterEx2(HWND,POINT,IDataObject*);
BOOL         WINAPI DAD_DragMove(POINT);
BOOL         WINAPI DAD_DragLeave(void);
BOOL         WINAPI DAD_AutoScroll(HWND,AUTO_SCROLL_DATA*,LPPOINT);
HRESULT      WINAPI SHDoDragDrop(HWND,IDataObject*,IDropSource*,DWORD,LPDWORD);

LPITEMIDLIST WINAPI ILAppendID(LPITEMIDLIST,LPCSHITEMID,BOOL);
LPITEMIDLIST WINAPI ILClone(LPCITEMIDLIST);
LPITEMIDLIST WINAPI ILCloneFirst(LPCITEMIDLIST);
LPITEMIDLIST WINAPI ILCreateFromPathA(LPCSTR);
LPITEMIDLIST WINAPI ILCreateFromPathW(LPCWSTR);
#define             ILCreateFromPath WINELIB_NAME_AW(ILCreateFromPath)
LPITEMIDLIST WINAPI ILCombine(LPCITEMIDLIST,LPCITEMIDLIST);
LPITEMIDLIST WINAPI ILFindChild(LPCITEMIDLIST,LPCITEMIDLIST);
LPITEMIDLIST WINAPI ILFindLastID(LPCITEMIDLIST);
void         WINAPI ILFree(LPITEMIDLIST);
LPITEMIDLIST WINAPI ILGetNext(LPCITEMIDLIST);
UINT         WINAPI ILGetSize(LPCITEMIDLIST);
BOOL         WINAPI ILIsEqual(LPCITEMIDLIST,LPCITEMIDLIST);
BOOL         WINAPI ILIsParent(LPCITEMIDLIST,LPCITEMIDLIST,BOOL);
HRESULT      WINAPI ILLoadFromStream(LPSTREAM,LPITEMIDLIST*);
BOOL         WINAPI ILRemoveLastID(LPITEMIDLIST);
HRESULT      WINAPI ILSaveToStream(LPSTREAM,LPCITEMIDLIST);


#include <poppack.h>

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_SHLOBJ_H */

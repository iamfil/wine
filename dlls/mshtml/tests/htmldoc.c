/*
 * Copyright 2005 Jacek Caban
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

#define COBJMACROS
#define CONST_VTABLE

#include <wine/test.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "mshtml.h"
#include "docobj.h"
#include "mshtmhst.h"
#include "mshtmdid.h"
#include "mshtmcid.h"
#include "hlink.h"
#include "idispids.h"
#include "shlguid.h"

#include "initguid.h"
DEFINE_SHLGUID(CGID_Undocumented, 0x000214D4L, 0, 0);

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        expect_ ## func = FALSE; \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

static IOleDocumentView *view = NULL;
static HWND container_hwnd = NULL, hwnd = NULL, last_hwnd = NULL;

DEFINE_EXPECT(LockContainer);
DEFINE_EXPECT(SetActiveObject);
DEFINE_EXPECT(GetWindow);
DEFINE_EXPECT(CanInPlaceActivate);
DEFINE_EXPECT(OnInPlaceActivate);
DEFINE_EXPECT(OnUIActivate);
DEFINE_EXPECT(GetWindowContext);
DEFINE_EXPECT(OnUIDeactivate);
DEFINE_EXPECT(OnInPlaceDeactivate);
DEFINE_EXPECT(GetContainer);
DEFINE_EXPECT(ShowUI);
DEFINE_EXPECT(ActivateMe);
DEFINE_EXPECT(GetHostInfo);
DEFINE_EXPECT(HideUI);
DEFINE_EXPECT(GetOptionKeyPath);
DEFINE_EXPECT(GetOverrideKeyPath);
DEFINE_EXPECT(SetStatusText);
DEFINE_EXPECT(QueryStatus_SETPROGRESSTEXT);
DEFINE_EXPECT(QueryStatus_OPEN);
DEFINE_EXPECT(QueryStatus_NEW);
DEFINE_EXPECT(Exec_SETPROGRESSMAX);
DEFINE_EXPECT(Exec_SETPROGRESSPOS);
DEFINE_EXPECT(Exec_HTTPEQUIV_DONE); 
DEFINE_EXPECT(Exec_SETDOWNLOADSTATE_0);
DEFINE_EXPECT(Exec_SETDOWNLOADSTATE_1);
DEFINE_EXPECT(Exec_ShellDocView_37);
DEFINE_EXPECT(Exec_UPDATECOMMANDS);
DEFINE_EXPECT(Exec_SETTITLE);
DEFINE_EXPECT(Exec_HTTPEQUIV);
DEFINE_EXPECT(Exec_MSHTML_PARSECOMPLETE);
DEFINE_EXPECT(Invoke_AMBIENT_USERMODE);
DEFINE_EXPECT(Invoke_AMBIENT_DLCONTROL);
DEFINE_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
DEFINE_EXPECT(Invoke_AMBIENT_SILENT);
DEFINE_EXPECT(Invoke_AMBIENT_USERAGENT);
DEFINE_EXPECT(Invoke_AMBIENT_PALETTE);
DEFINE_EXPECT(GetDropTarget);
DEFINE_EXPECT(UpdateUI);
DEFINE_EXPECT(Navigate);
DEFINE_EXPECT(OnFrameWindowActivate);
DEFINE_EXPECT(OnChanged_READYSTATE);
DEFINE_EXPECT(OnChanged_1005);
DEFINE_EXPECT(GetDisplayName);
DEFINE_EXPECT(BindToStorage);
DEFINE_EXPECT(Abort);

static BOOL expect_LockContainer_fLock;
static BOOL expect_SetActiveObject_active;
static BOOL set_clientsite = FALSE, container_locked = FALSE;
static BOOL readystate_set_loading = FALSE;
static enum load_state_t {
    LD_DOLOAD,
    LD_LOADING,
    LD_INTERACTIVE,
    LD_COMPLETE,
    LD_NO
} load_state;

static LPCOLESTR expect_status_text = NULL;

static HRESULT QueryInterface(REFIID riid, void **ppv);
static void test_readyState(IUnknown*);

static HRESULT WINAPI HlinkFrame_QueryInterface(IHlinkFrame *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI HlinkFrame_AddRef(IHlinkFrame *iface)
{
    return 2;
}

static ULONG WINAPI HlinkFrame_Release(IHlinkFrame *iface)
{
    return 1;
}

static HRESULT WINAPI HlinkFrame_SetBrowseContext(IHlinkFrame *iface,
                                                  IHlinkBrowseContext *pihlbc)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_GetBrowseContext(IHlinkFrame *iface,
                                                  IHlinkBrowseContext **ppihlbc)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_Navigate(IHlinkFrame *iface, DWORD grfHLNF, LPBC pbc,
                                          IBindStatusCallback *pibsc, IHlink *pihlNavigate)
{
    HRESULT hres;

    CHECK_EXPECT(Navigate);

    ok(grfHLNF == 0, "grfHLNF=%ld, expected 0\n", grfHLNF);
    ok(pbc != NULL, "pbc == NULL\n");
    ok(pibsc != NULL, "pubsc == NULL\n");
    ok(pihlNavigate != NULL, "puhlNavigate == NULL\n");

    if(pihlNavigate) {
        LPWSTR frame_name = (LPWSTR)0xdeadbeef;
        LPWSTR location = (LPWSTR)0xdeadbeef;
        IHlinkSite *site;
        IMoniker *mon = NULL;
        DWORD site_data = 0xdeadbeef;

        hres = IHlink_GetTargetFrameName(pihlNavigate, &frame_name);
        ok(hres == S_FALSE, "GetTargetFrameName failed: %08lx\n", hres);
        ok(frame_name == NULL, "frame_name = %p\n", frame_name);

        hres = IHlink_GetMonikerReference(pihlNavigate, 1, &mon, &location);
        ok(hres == S_OK, "GetMonikerReference failed: %08lx\n", hres);
        ok(location == NULL, "location = %p\n", location);
        ok(mon != NULL, "mon == NULL\n");

        hres = IHlink_GetHlinkSite(pihlNavigate, &site, &site_data);
        ok(hres == S_OK, "GetHlinkSite failed: %08lx\n", hres);
        ok(site == NULL, "site = %p\n, expected NULL\n", site);
        ok(site_data == 0xdeadbeef, "site_data = %lx\n", site_data);
    }

    return S_OK;
}

static HRESULT WINAPI HlinkFrame_OnNavigate(IHlinkFrame *iface, DWORD grfHLNF,
        IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName, DWORD dwreserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_UpdateHlink(IHlinkFrame *iface, ULONG uHLID,
        IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IHlinkFrameVtbl HlinkFrameVtbl = {
    HlinkFrame_QueryInterface,
    HlinkFrame_AddRef,
    HlinkFrame_Release,
    HlinkFrame_SetBrowseContext,
    HlinkFrame_GetBrowseContext,
    HlinkFrame_Navigate,
    HlinkFrame_OnNavigate,
    HlinkFrame_UpdateHlink
};

static IHlinkFrame HlinkFrame = { &HlinkFrameVtbl };

static HRESULT WINAPI PropertyNotifySink_QueryInterface(IPropertyNotifySink *iface,
        REFIID riid, void**ppv)
{
    if(IsEqualGUID(&IID_IPropertyNotifySink, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI PropertyNotifySink_AddRef(IPropertyNotifySink *iface)
{
    return 2;
}

static ULONG WINAPI PropertyNotifySink_Release(IPropertyNotifySink *iface)
{
    return 1;
}

static HRESULT WINAPI PropertyNotifySink_OnChanged(IPropertyNotifySink *iface, DISPID dispID)
{
    switch(dispID) {
    case DISPID_READYSTATE:
        CHECK_EXPECT2(OnChanged_READYSTATE);
        if(readystate_set_loading) {
            readystate_set_loading = FALSE;
            load_state = LD_LOADING;
        }
        test_readyState(NULL);
        return S_OK;
    case 1005:
        CHECK_EXPECT(OnChanged_1005);
        test_readyState(NULL);
        load_state = LD_INTERACTIVE;
        return S_OK;
    }

    ok(0, "unexpected id %ld\n", dispID);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyNotifySink_OnRequestEdit(IPropertyNotifySink *iface, DISPID dispID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IPropertyNotifySinkVtbl PropertyNotifySinkVtbl = {
    PropertyNotifySink_QueryInterface,
    PropertyNotifySink_AddRef,
    PropertyNotifySink_Release,
    PropertyNotifySink_OnChanged,
    PropertyNotifySink_OnRequestEdit
};

static IPropertyNotifySink PropertyNotifySink = { &PropertyNotifySinkVtbl };

static HRESULT WINAPI Binding_QueryInterface(IBinding *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IWinInetHttpInfo, riid))
        return E_NOINTERFACE; /* TODO */

    if(IsEqualGUID(&IID_IWinInetInfo, riid))
        return E_NOINTERFACE; /* TODO */

    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI Binding_AddRef(IBinding *iface)
{
    return 2;
}

static ULONG WINAPI Binding_Release(IBinding *iface)
{
    return 1;
}

static HRESULT WINAPI Binding_Abort(IBinding *iface)
{
    CHECK_EXPECT(Abort);
    return S_OK;
}

static HRESULT WINAPI Binding_Suspend(IBinding *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_Resume(IBinding *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_SetPriority(IBinding *iface, LONG nPriority)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_GetPriority(IBinding *iface, LONG *pnPriority)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_GetBindResult(IBinding *iface, CLSID *pclsidProtocol,
        DWORD *pdwResult, LPOLESTR *pszResult, DWORD *pdwReserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IBindingVtbl BindingVtbl = {
    Binding_QueryInterface,
    Binding_AddRef,
    Binding_Release,
    Binding_Abort,
    Binding_Suspend,
    Binding_Resume,
    Binding_SetPriority,
    Binding_GetPriority,
    Binding_GetBindResult
};

static IBinding Binding = { &BindingVtbl };

static HRESULT WINAPI Moniker_QueryInterface(IMoniker *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI Moniker_AddRef(IMoniker *iface)
{
    return 2;
}

static ULONG WINAPI Moniker_Release(IMoniker *iface)
{
    return 1;
}

static HRESULT WINAPI Moniker_GetClassID(IMoniker *iface, CLSID *pClassID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_IsDirty(IMoniker *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_Load(IMoniker *iface, IStream *pStm)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_Save(IMoniker *iface, IStream *pStm, BOOL fClearDirty)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_GetSizeMax(IMoniker *iface, ULARGE_INTEGER *pcbSize)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_BindToObject(IMoniker *iface, IBindCtx *pcb, IMoniker *pmkToLeft,
        REFIID riidResult, void **ppvResult)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_BindToStorage(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft,
        REFIID riid, void **ppv)
{
    IBindStatusCallback *callback = NULL;
    FORMATETC formatetc = {0xc02d, NULL, 1, -1, TYMED_ISTREAM};
    STGMEDIUM stgmedium;
    BINDINFO bindinfo;
    DWORD bindf, written=0;
    IStream *stream;
    HRESULT hres;

    static OLECHAR BSCBHolder[] = { '_','B','S','C','B','_','H','o','l','d','e','r','_',0 };
    static const WCHAR wszTextHtml[] = {'t','e','x','t','/','h','t','m','l',0};
    static const char html_page[] = "<html><body>test</body></html>";

    CHECK_EXPECT(BindToStorage);

    ok(pbc != NULL, "pbc == NULL\n");
    ok(pmkToLeft == NULL, "pmkToLeft=%p\n", pmkToLeft);
    ok(IsEqualGUID(&IID_IStream, riid), "unexpected riid\n");
    ok(ppv != NULL, "ppv == NULL\n");
    ok(*ppv == NULL, "*ppv=%p\n", *ppv);

    hres = IBindCtx_GetObjectParam(pbc, BSCBHolder, (IUnknown**)&callback);
    ok(hres == S_OK, "GetObjectParam failed: %08lx\n", hres);
    ok(callback != NULL, "callback == NULL\n");

    memset(&bindinfo, 0xf0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(bindinfo);

    hres = IBindStatusCallback_GetBindInfo(callback, &bindf, &bindinfo);
    ok(hres == S_OK, "GetBindInfo failed: %08lx\n", hres);
    ok(bindf == (BINDF_PULLDATA|BINDF_ASYNCSTORAGE|BINDF_ASYNCHRONOUS), "bindf = %08lx\n", bindf);
    ok(bindinfo.cbSize == sizeof(bindinfo), "bindinfo.cbSize=%ld\n", bindinfo.cbSize);
    ok(bindinfo.szExtraInfo == NULL, "bindinfo.szExtraInfo=%p\n", bindinfo.szExtraInfo);
    /* TODO: test stgmedData */
    ok(bindinfo.grfBindInfoF == 0, "bindinfo.grfBinfInfoF=%08lx\n", bindinfo.grfBindInfoF);
    ok(bindinfo.dwBindVerb == 0, "bindinfo.dwBindVerb=%ld\n", bindinfo.dwBindVerb);
    ok(bindinfo.szCustomVerb == 0, "bindinfo.szCustomVerb=%p\n", bindinfo.szCustomVerb);
    ok(bindinfo.cbstgmedData == 0, "bindinfo.cbstgmedData=%ld\n", bindinfo.cbstgmedData);
    ok(bindinfo.dwOptions == 0x80000, "bindinfo.dwOptions=%lx\n", bindinfo.dwOptions);
    ok(bindinfo.dwOptionsFlags == 0, "bindinfo.dwOptionsFlags=%ld\n", bindinfo.dwOptionsFlags);
    /* TODO: test dwCodePage */
    /* TODO: test securityAttributes */
    ok(IsEqualGUID(&IID_NULL, &bindinfo.iid), "unexepected bindinfo.iid\n");
    ok(bindinfo.pUnk == NULL, "bindinfo.pUnk=%p\n", bindinfo.pUnk);
    ok(bindinfo.dwReserved == 0, "bindinfo.dwReserved=%ld\n", bindinfo.dwReserved);

    hres = IBindStatusCallback_OnStartBinding(callback, 0, &Binding);
    ok(hres == S_OK, "OnStartBinding failed: %08lx\n", hres);

    hres = IBindStatusCallback_OnProgress(callback, 0, 0, BINDSTATUS_MIMETYPEAVAILABLE,
                                          wszTextHtml);
    ok(hres == S_OK, "OnProgress(BINDSTATUS_MIMETYPEAVAILABLE) failed: %08lx\n", hres);

    hres = IBindStatusCallback_OnProgress(callback, 0, 0, BINDSTATUS_BEGINDOWNLOADDATA,
                                          NULL);
    ok(hres == S_OK, "OnProgress(BINDSTATUS_BEGINDOWNLOADDATA) failed: %08lx\n", hres);

    CreateStreamOnHGlobal(0, TRUE, &stream);
    IStream_Write(stream, html_page, sizeof(html_page)-1, &written);

    stgmedium.tymed = TYMED_ISTREAM;
    U(stgmedium).pstm = stream;
    stgmedium.pUnkForRelease = (IUnknown*)iface;
    hres = IBindStatusCallback_OnDataAvailable(callback,
            BSCF_FIRSTDATANOTIFICATION|BSCF_LASTDATANOTIFICATION,
            100, &formatetc, &stgmedium);
    ok(hres == S_OK, "OnDataAvailable failed: %08lx\n", hres);
    IStream_Release(stream);

    hres = IBindStatusCallback_OnProgress(callback, sizeof(html_page)-1, sizeof(html_page)-1,
            BINDSTATUS_ENDDOWNLOADDATA, NULL);
    ok(hres == S_OK, "OnProgress(BINDSTATUS_ENDDOWNLOADDATA) failed: %08lx\n", hres);

    hres = IBindStatusCallback_OnStopBinding(callback, S_OK, NULL);
    ok(hres == S_OK, "OnStopBinding failed: %08lx\n", hres);

    IBindStatusCallback_Release(callback);

    return S_OK;
}

static HRESULT WINAPI Moniker_Reduce(IMoniker *iface, IBindCtx *pbc, DWORD dwReduceHowFar,
        IMoniker **ppmkToLeft, IMoniker **ppmkReduced)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_ComposeWith(IMoniker *iface, IMoniker *pmkRight,
        BOOL fOnlyIfNotGeneric, IMoniker **ppnkComposite)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_Enum(IMoniker *iface, BOOL fForwrd, IEnumMoniker **ppenumMoniker)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_IsEqual(IMoniker *iface, IMoniker *pmkOtherMoniker)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_Hash(IMoniker *iface, DWORD *pdwHash)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_IsRunning(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft,
        IMoniker *pmkNewlyRunning)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_GetTimeOfLastChange(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, FILETIME *pFileTime)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_Inverse(IMoniker *iface, IMoniker **ppmk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_CommonPrefixWith(IMoniker *iface, IMoniker *pmkOther,
        IMoniker **ppmkPrefix)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_RelativePathTo(IMoniker *iface, IMoniker *pmkOther,
        IMoniker **pmkRelPath)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_GetDisplayName(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, LPOLESTR *ppszDisplayName)
{
    static const WCHAR winetest_test[] =
        {'w','i','n','e','t','e','s','t',':','t','e','s','t',0};

    CHECK_EXPECT2(GetDisplayName);

    /* ok(pbc != NULL, "pbc == NULL\n"); */
    ok(pmkToLeft == NULL, "pmkToLeft=%p\n", pmkToLeft);
    ok(ppszDisplayName != NULL, "ppszDisplayName == NULL\n");
    ok(*ppszDisplayName == NULL, "*ppszDisplayName=%p\n", *ppszDisplayName);

    *ppszDisplayName = CoTaskMemAlloc(sizeof(winetest_test));
    memcpy(*ppszDisplayName, winetest_test, sizeof(winetest_test));

    return S_OK;
}

static HRESULT WINAPI Moniker_ParseDisplayName(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, LPOLESTR pszDisplayName, ULONG *pchEaten, IMoniker **ppmkOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_IsSystemMoniker(IMoniker *iface, DWORD *pdwMksys)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IMonikerVtbl MonikerVtbl = {
    Moniker_QueryInterface,
    Moniker_AddRef,
    Moniker_Release,
    Moniker_GetClassID,
    Moniker_IsDirty,
    Moniker_Load,
    Moniker_Save,
    Moniker_GetSizeMax,
    Moniker_BindToObject,
    Moniker_BindToStorage,
    Moniker_Reduce,
    Moniker_ComposeWith,
    Moniker_Enum,
    Moniker_IsEqual,
    Moniker_Hash,
    Moniker_IsRunning,
    Moniker_GetTimeOfLastChange,
    Moniker_Inverse,
    Moniker_CommonPrefixWith,
    Moniker_RelativePathTo,
    Moniker_GetDisplayName,
    Moniker_ParseDisplayName,
    Moniker_IsSystemMoniker
};

static IMoniker Moniker = { &MonikerVtbl };

static HRESULT WINAPI OleContainer_QueryInterface(IOleContainer *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI OleContainer_AddRef(IOleContainer *iface)
{
    return 2;
}

static ULONG WINAPI OleContainer_Release(IOleContainer *iface)
{
    return 1;
}

static HRESULT WINAPI OleContainer_ParseDisplayName(IOleContainer *iface, IBindCtx *pbc,
        LPOLESTR pszDiaplayName, ULONG *pchEaten, IMoniker **ppmkOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleContainer_EnumObjects(IOleContainer *iface, DWORD grfFlags,
        IEnumUnknown **ppenum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleContainer_LockContainer(IOleContainer *iface, BOOL fLock)
{
    CHECK_EXPECT(LockContainer);
    ok(expect_LockContainer_fLock == fLock, "fLock=%x, expected %x\n", fLock, expect_LockContainer_fLock);
    return S_OK;
}

static const IOleContainerVtbl OleContainerVtbl = {
    OleContainer_QueryInterface,
    OleContainer_AddRef,
    OleContainer_Release,
    OleContainer_ParseDisplayName,
    OleContainer_EnumObjects,
    OleContainer_LockContainer
};

static IOleContainer OleContainer = { &OleContainerVtbl };

static HRESULT WINAPI InPlaceFrame_QueryInterface(IOleInPlaceFrame *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI InPlaceFrame_AddRef(IOleInPlaceFrame *iface)
{
    return 2;
}

static ULONG WINAPI InPlaceFrame_Release(IOleInPlaceFrame *iface)
{
    return 1;
}

static HRESULT WINAPI InPlaceFrame_GetWindow(IOleInPlaceFrame *iface, HWND *phwnd)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_ContextSensitiveHelp(IOleInPlaceFrame *iface, BOOL fEnterMode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_GetBorder(IOleInPlaceFrame *iface, LPRECT lprectBorder)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_RequestBorderSpace(IOleInPlaceFrame *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetBorderSpace(IOleInPlaceFrame *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    static const WCHAR wszHTML_Document[] =
        {'H','T','M','L',' ','D','o','c','u','m','e','n','t',0};

    CHECK_EXPECT2(SetActiveObject);

    if(expect_SetActiveObject_active) {
        ok(pActiveObject != NULL, "pActiveObject = NULL\n");
        if(pActiveObject && PRIMARYLANGID(GetSystemDefaultLangID()) == LANG_ENGLISH)
            ok(!lstrcmpW(wszHTML_Document, pszObjName), "pszObjName != \"HTML Document\"\n");
    }else {
        ok(pActiveObject == NULL, "pActiveObject=%p, expected NULL\n", pActiveObject);
        ok(pszObjName == NULL, "pszObjName=%p, expected NULL\n", pszObjName);
    }

    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_InsertMenus(IOleInPlaceFrame *iface, HMENU hmenuShared,
        LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetMenu(IOleInPlaceFrame *iface, HMENU hmenuShared,
        HOLEMENU holemenu, HWND hwndActiveObject)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_RemoveMenus(IOleInPlaceFrame *iface, HMENU hmenuShared)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetStatusText(IOleInPlaceFrame *iface, LPCOLESTR pszStatusText)
{
    CHECK_EXPECT2(SetStatusText);
    if(!expect_status_text)
        ok(pszStatusText == NULL, "pszStatusText=%p, expected NULL\n", pszStatusText);
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_EnableModeless(IOleInPlaceFrame *iface, BOOL fEnable)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_TranslateAccelerator(IOleInPlaceFrame *iface, LPMSG lpmsg, WORD wID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleInPlaceFrameVtbl InPlaceFrameVtbl = {
    InPlaceFrame_QueryInterface,
    InPlaceFrame_AddRef,
    InPlaceFrame_Release,
    InPlaceFrame_GetWindow,
    InPlaceFrame_ContextSensitiveHelp,
    InPlaceFrame_GetBorder,
    InPlaceFrame_RequestBorderSpace,
    InPlaceFrame_SetBorderSpace,
    InPlaceFrame_SetActiveObject,
    InPlaceFrame_InsertMenus,
    InPlaceFrame_SetMenu,
    InPlaceFrame_RemoveMenus,
    InPlaceFrame_SetStatusText,
    InPlaceFrame_EnableModeless,
    InPlaceFrame_TranslateAccelerator
};

static IOleInPlaceFrame InPlaceFrame = { &InPlaceFrameVtbl };

static HRESULT WINAPI InPlaceSite_QueryInterface(IOleInPlaceSite *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI InPlaceSite_AddRef(IOleInPlaceSite *iface)
{
    return 2;
}

static ULONG WINAPI InPlaceSite_Release(IOleInPlaceSite *iface)
{
    return 1;
}

static HRESULT WINAPI InPlaceSite_GetWindow(IOleInPlaceSite *iface, HWND *phwnd)
{
    CHECK_EXPECT(GetWindow);
    ok(phwnd != NULL, "phwnd = NULL\n");
    *phwnd = container_hwnd;
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_ContextSensitiveHelp(IOleInPlaceSite *iface, BOOL fEnterMode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_CanInPlaceActivate(IOleInPlaceSite *iface)
{
    CHECK_EXPECT(CanInPlaceActivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceActivate(IOleInPlaceSite *iface)
{
    CHECK_EXPECT(OnInPlaceActivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnUIActivate(IOleInPlaceSite *iface)
{
    CHECK_EXPECT(OnUIActivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_GetWindowContext(IOleInPlaceSite *iface,
        IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc, LPRECT lprcPosRect,
        LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    static const RECT rect = {0,0,500,500};

    CHECK_EXPECT(GetWindowContext);

    ok(ppFrame != NULL, "ppFrame = NULL\n");
    if(ppFrame)
        *ppFrame = &InPlaceFrame;
    ok(ppDoc != NULL, "ppDoc = NULL\n");
    if(ppDoc)
        *ppDoc = NULL;
    ok(lprcPosRect != NULL, "lprcPosRect = NULL\n");
    if(lprcPosRect)
        memcpy(lprcPosRect, &rect, sizeof(RECT));
    ok(lprcClipRect != NULL, "lprcClipRect = NULL\n");
    if(lprcClipRect)
        memcpy(lprcClipRect, &rect, sizeof(RECT));
    ok(lpFrameInfo != NULL, "lpFrameInfo = NULL\n");
    if(lpFrameInfo) {
        lpFrameInfo->cb = sizeof(*lpFrameInfo);
        lpFrameInfo->fMDIApp = FALSE;
        lpFrameInfo->hwndFrame = container_hwnd;
        lpFrameInfo->haccel = NULL;
        lpFrameInfo->cAccelEntries = 0;
    }

    return S_OK;
}

static HRESULT WINAPI InPlaceSite_Scroll(IOleInPlaceSite *iface, SIZE scrollExtant)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnUIDeactivate(IOleInPlaceSite *iface, BOOL fUndoable)
{
    CHECK_EXPECT(OnUIDeactivate);
    ok(!fUndoable, "fUndoable = TRUE\n");
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceDeactivate(IOleInPlaceSite *iface)
{
    CHECK_EXPECT(OnInPlaceDeactivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_DiscardUndoState(IOleInPlaceSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_DeactivateAndUndo(IOleInPlaceSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnPosRectChange(IOleInPlaceSite *iface, LPCRECT lprcPosRect)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleInPlaceSiteVtbl InPlaceSiteVtbl = {
    InPlaceSite_QueryInterface,
    InPlaceSite_AddRef,
    InPlaceSite_Release,
    InPlaceSite_GetWindow,
    InPlaceSite_ContextSensitiveHelp,
    InPlaceSite_CanInPlaceActivate,
    InPlaceSite_OnInPlaceActivate,
    InPlaceSite_OnUIActivate,
    InPlaceSite_GetWindowContext,
    InPlaceSite_Scroll,
    InPlaceSite_OnUIDeactivate,
    InPlaceSite_OnInPlaceDeactivate,
    InPlaceSite_DiscardUndoState,
    InPlaceSite_DeactivateAndUndo,
    InPlaceSite_OnPosRectChange
};

static IOleInPlaceSite InPlaceSite = { &InPlaceSiteVtbl };

static HRESULT WINAPI ClientSite_QueryInterface(IOleClientSite *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI ClientSite_AddRef(IOleClientSite *iface)
{
    return 2;
}

static ULONG WINAPI ClientSite_Release(IOleClientSite *iface)
{
    return 1;
}

static HRESULT WINAPI ClientSite_SaveObject(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetMoniker(IOleClientSite *iface, DWORD dwAsign, DWORD dwWhichMoniker,
        IMoniker **ppmon)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetContainer(IOleClientSite *iface, IOleContainer **ppContainer)
{
    CHECK_EXPECT(GetContainer);
    ok(ppContainer != NULL, "ppContainer = NULL\n");
    *ppContainer = &OleContainer;
    return S_OK;
}

static HRESULT WINAPI ClientSite_ShowObject(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_OnShowWindow(IOleClientSite *iface, BOOL fShow)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_RequestNewObjectLayout(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleClientSiteVtbl ClientSiteVtbl = {
    ClientSite_QueryInterface,
    ClientSite_AddRef,
    ClientSite_Release,
    ClientSite_SaveObject,
    ClientSite_GetMoniker,
    ClientSite_GetContainer,
    ClientSite_ShowObject,
    ClientSite_OnShowWindow,
    ClientSite_RequestNewObjectLayout
};

static IOleClientSite ClientSite = { &ClientSiteVtbl };

static HRESULT WINAPI DocumentSite_QueryInterface(IOleDocumentSite *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI DocumentSite_AddRef(IOleDocumentSite *iface)
{
    return 2;
}

static ULONG WINAPI DocumentSite_Release(IOleDocumentSite *iface)
{
    return 1;
}

static BOOL call_UIActivate = TRUE;
static HRESULT WINAPI DocumentSite_ActivateMe(IOleDocumentSite *iface, IOleDocumentView *pViewToActivate)
{
    IOleDocument *document;
    HRESULT hres;

    CHECK_EXPECT(ActivateMe);
    ok(pViewToActivate != NULL, "pViewToActivate = NULL\n");

    hres = IOleDocumentView_QueryInterface(pViewToActivate, &IID_IOleDocument, (void**)&document);
    ok(hres == S_OK, "could not get IOleDocument: %08lx\n", hres);

    if(SUCCEEDED(hres)) {
        hres = IOleDocument_CreateView(document, &InPlaceSite, NULL, 0, &view);
        ok(hres == S_OK, "CreateView failed: %08lx\n", hres);

        if(SUCCEEDED(hres)) {
            IOleInPlaceActiveObject *activeobj = NULL;
            IOleInPlaceSite *inplacesite = NULL;
            HWND tmp_hwnd = NULL;
            static RECT rect = {0,0,400,500};

            hres = IOleDocumentView_GetInPlaceSite(view, &inplacesite);
            ok(hres == S_OK, "GetInPlaceSite failed: %08lx\n", hres);
            ok(inplacesite == &InPlaceSite, "inplacesite=%p, expected %p\n",
                    inplacesite, &InPlaceSite);

            hres = IOleDocumentView_SetInPlaceSite(view, &InPlaceSite);
            ok(hres == S_OK, "SetInPlaceSite failed: %08lx\n", hres);

            hres = IOleDocumentView_GetInPlaceSite(view, &inplacesite);
            ok(hres == S_OK, "GetInPlaceSite failed: %08lx\n", hres);
            ok(inplacesite == &InPlaceSite, "inplacesite=%p, expected %p\n",
                    inplacesite, &InPlaceSite);

            hres = IOleDocumentView_QueryInterface(view, &IID_IOleInPlaceActiveObject, (void**)&activeobj);
            ok(hres == S_OK, "Could not get IOleInPlaceActiveObject: %08lx\n", hres);

            if(activeobj) {
                IOleInPlaceActiveObject_GetWindow(activeobj, &hwnd);
                ok(hres == S_OK, "GetWindow failed: %08lx\n", hres);
                ok(hwnd == NULL, "hwnd=%p, expeted NULL\n", hwnd);
            }
            
            if(call_UIActivate) {
                SET_EXPECT(CanInPlaceActivate);
                SET_EXPECT(GetWindowContext);
                SET_EXPECT(GetWindow);
                SET_EXPECT(OnInPlaceActivate);
                SET_EXPECT(SetStatusText);
                SET_EXPECT(Exec_SETPROGRESSMAX);
                SET_EXPECT(Exec_SETPROGRESSPOS);
                SET_EXPECT(OnUIActivate);
                SET_EXPECT(SetActiveObject);
                SET_EXPECT(ShowUI);
                expect_SetActiveObject_active = TRUE;
                expect_status_text = NULL;

                hres = IOleDocumentView_UIActivate(view, TRUE);

                if(FAILED(hres)) {
                    trace("UIActivate failed: %08lx\n", hres);
                    return hres;
                }
                ok(hres == S_OK, "UIActivate failed: %08lx\n", hres);

                CHECK_CALLED(CanInPlaceActivate);
                CHECK_CALLED(GetWindowContext);
                CHECK_CALLED(GetWindow);
                CHECK_CALLED(OnInPlaceActivate);
                CHECK_CALLED(SetStatusText);
                CHECK_CALLED(Exec_SETPROGRESSMAX);
                CHECK_CALLED(Exec_SETPROGRESSPOS);
                CHECK_CALLED(OnUIActivate);
                CHECK_CALLED(SetActiveObject);
                CHECK_CALLED(ShowUI);

                if(activeobj) {
                    hres = IOleInPlaceActiveObject_GetWindow(activeobj, &hwnd);
                    ok(hres == S_OK, "GetWindow failed: %08lx\n", hres);
                    ok(hwnd != NULL, "hwnd == NULL\n");
                    if(last_hwnd)
                        ok(hwnd == last_hwnd, "hwnd != last_hwnd\n");
                }

                hres = IOleDocumentView_UIActivate(view, TRUE);
                ok(hres == S_OK, "UIActivate failed: %08lx\n", hres);

                if(activeobj) {
                    hres = IOleInPlaceActiveObject_GetWindow(activeobj, &tmp_hwnd);
                    ok(hres == S_OK, "GetWindow failed: %08lx\n", hres);
                    ok(tmp_hwnd == hwnd, "tmp_hwnd=%p, expected %p\n", tmp_hwnd, hwnd);
                }
            }

            hres = IOleDocumentView_SetRect(view, &rect);
            ok(hres == S_OK, "SetRect failed: %08lx\n", hres);

            if(call_UIActivate) {
                hres = IOleDocumentView_Show(view, TRUE);
                ok(hres == S_OK, "Show failed: %08lx\n", hres);
            }else {
                SET_EXPECT(CanInPlaceActivate);
                SET_EXPECT(GetWindowContext);
                SET_EXPECT(GetWindow);
                SET_EXPECT(OnInPlaceActivate);
                SET_EXPECT(SetStatusText);
                SET_EXPECT(Exec_SETPROGRESSMAX);
                SET_EXPECT(Exec_SETPROGRESSPOS);
                SET_EXPECT(OnUIActivate);
                expect_status_text = (load_state == LD_COMPLETE ? (LPCOLESTR)0xdeadbeef : NULL);

                hres = IOleDocumentView_Show(view, TRUE);
                ok(hres == S_OK, "Show failed: %08lx\n", hres);

                CHECK_CALLED(CanInPlaceActivate);
                CHECK_CALLED(GetWindowContext);
                CHECK_CALLED(GetWindow);
                CHECK_CALLED(OnInPlaceActivate);
                CHECK_CALLED(SetStatusText);
                CHECK_CALLED(Exec_SETPROGRESSMAX);
                CHECK_CALLED(Exec_SETPROGRESSPOS);

                if(activeobj) {
                    hres = IOleInPlaceActiveObject_GetWindow(activeobj, &hwnd);
                    ok(hres == S_OK, "GetWindow failed: %08lx\n", hres);
                    ok(hwnd != NULL, "hwnd == NULL\n");
                    if(last_hwnd)
                        ok(hwnd == last_hwnd, "hwnd != last_hwnd\n");
                }
            }

            if(activeobj)
                IOleInPlaceActiveObject_Release(activeobj);
        }

        IOleDocument_Release(document);
    }

    return S_OK;
}

static const IOleDocumentSiteVtbl DocumentSiteVtbl = {
    DocumentSite_QueryInterface,
    DocumentSite_AddRef,
    DocumentSite_Release,
    DocumentSite_ActivateMe
};

static IOleDocumentSite DocumentSite = { &DocumentSiteVtbl };

static HRESULT WINAPI DocHostUIHandler_QueryInterface(IDocHostUIHandler2 *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI DocHostUIHandler_AddRef(IDocHostUIHandler2 *iface)
{
    return 2;
}

static ULONG WINAPI DocHostUIHandler_Release(IDocHostUIHandler2 *iface)
{
    return 1;
}

static HRESULT WINAPI DocHostUIHandler_ShowContextMenu(IDocHostUIHandler2 *iface, DWORD dwID, POINT *ppt,
        IUnknown *pcmdtReserved, IDispatch *pdicpReserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetHostInfo(IDocHostUIHandler2 *iface, DOCHOSTUIINFO *pInfo)
{
    CHECK_EXPECT(GetHostInfo);
    ok(pInfo != NULL, "pInfo=NULL\n");
    if(pInfo) {
        ok(pInfo->cbSize == sizeof(DOCHOSTUIINFO), "pInfo->cbSize=%lu\n", pInfo->cbSize);
        ok(!pInfo->dwFlags, "pInfo->dwFlags=%08lx, expected 0\n", pInfo->dwFlags);
        pInfo->dwFlags = DOCHOSTUIFLAG_DISABLE_HELP_MENU | DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE
            | DOCHOSTUIFLAG_ACTIVATE_CLIENTHIT_ONLY | DOCHOSTUIFLAG_ENABLE_INPLACE_NAVIGATION
            | DOCHOSTUIFLAG_IME_ENABLE_RECONVERSION;
        ok(!pInfo->dwDoubleClick, "pInfo->dwDoubleClick=%08lx, expected 0\n", pInfo->dwDoubleClick);
        ok(!pInfo->pchHostCss, "pInfo->pchHostCss=%p, expected NULL\n", pInfo->pchHostCss);
        ok(!pInfo->pchHostNS, "pInfo->pchhostNS=%p, expected NULL\n", pInfo->pchHostNS);
    }
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_ShowUI(IDocHostUIHandler2 *iface, DWORD dwID,
        IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget,
        IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
{
    CHECK_EXPECT(ShowUI);

    ok(dwID == DOCHOSTUITYPE_BROWSE, "dwID=%ld, expected DOCHOSTUITYPE_BROWSE\n", dwID);
    ok(pActiveObject != NULL, "pActiveObject = NULL\n");
    ok(pCommandTarget != NULL, "pCommandTarget = NULL\n");
    ok(pFrame == &InPlaceFrame, "pFrame=%p, expected %p\n", pFrame, &InPlaceFrame);
    ok(pDoc == NULL, "pDoc=%p, expected NULL\n", pDoc);

    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_HideUI(IDocHostUIHandler2 *iface)
{
    CHECK_EXPECT(HideUI);
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_UpdateUI(IDocHostUIHandler2 *iface)
{
    CHECK_EXPECT(UpdateUI);
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_EnableModeless(IDocHostUIHandler2 *iface, BOOL fEnable)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_OnDocWindowActivate(IDocHostUIHandler2 *iface, BOOL fActivate)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static BOOL expect_OnFrameWindowActivate_fActivate;
static HRESULT WINAPI DocHostUIHandler_OnFrameWindowActivate(IDocHostUIHandler2 *iface, BOOL fActivate)
{
    CHECK_EXPECT2(OnFrameWindowActivate);
    ok(fActivate == expect_OnFrameWindowActivate_fActivate, "fActivate=%x\n", fActivate);
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_ResizeBorder(IDocHostUIHandler2 *iface, LPCRECT prcBorder,
        IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_TranslateAccelerator(IDocHostUIHandler2 *iface, LPMSG lpMsg,
        const GUID *pguidCmdGroup, DWORD nCmdID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetOptionKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    CHECK_EXPECT(GetOptionKeyPath);
    ok(pchKey != NULL, "pchKey = NULL\n");
    ok(!dw, "dw=%ld, expected 0\n", dw);
    if(pchKey)
        ok(!*pchKey, "*pchKey=%p, expected NULL\n", *pchKey);
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_GetDropTarget(IDocHostUIHandler2 *iface,
        IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
    CHECK_EXPECT(GetDropTarget);
    /* TODO */
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetExternal(IDocHostUIHandler2 *iface, IDispatch **ppDispatch)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_TranslateUrl(IDocHostUIHandler2 *iface, DWORD dwTranslate,
        OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_FilterDataObject(IDocHostUIHandler2 *iface, IDataObject *pDO,
        IDataObject **ppPORet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetOverrideKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    CHECK_EXPECT(GetOverrideKeyPath);
    ok(pchKey != NULL, "pchKey = NULL\n");
    if(pchKey)
        ok(!*pchKey, "*pchKey=%p, expected NULL\n", *pchKey);
    ok(!dw, "dw=%ld, xepected 0\n", dw);
    return S_OK;
}

static const IDocHostUIHandler2Vtbl DocHostUIHandlerVtbl = {
    DocHostUIHandler_QueryInterface,
    DocHostUIHandler_AddRef,
    DocHostUIHandler_Release,
    DocHostUIHandler_ShowContextMenu,
    DocHostUIHandler_GetHostInfo,
    DocHostUIHandler_ShowUI,
    DocHostUIHandler_HideUI,
    DocHostUIHandler_UpdateUI,
    DocHostUIHandler_EnableModeless,
    DocHostUIHandler_OnDocWindowActivate,
    DocHostUIHandler_OnFrameWindowActivate,
    DocHostUIHandler_ResizeBorder,
    DocHostUIHandler_TranslateAccelerator,
    DocHostUIHandler_GetOptionKeyPath,
    DocHostUIHandler_GetDropTarget,
    DocHostUIHandler_GetExternal,
    DocHostUIHandler_TranslateUrl,
    DocHostUIHandler_FilterDataObject,
    DocHostUIHandler_GetOverrideKeyPath
};

static IDocHostUIHandler2 DocHostUIHandler = { &DocHostUIHandlerVtbl };

static HRESULT WINAPI OleCommandTarget_QueryInterface(IOleCommandTarget *iface,
        REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI OleCommandTarget_AddRef(IOleCommandTarget *iface)
{
    return 2;
}

static ULONG WINAPI OleCommandTarget_Release(IOleCommandTarget *iface)
{
    return 1;
}

static HRESULT WINAPI OleCommandTarget_QueryStatus(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    ok(!pguidCmdGroup, "pguidCmdGroup != MULL\n");
    ok(cCmds == 1, "cCmds=%ld, expected 1\n", cCmds);
    ok(!pCmdText, "pCmdText != NULL\n");

    switch(prgCmds[0].cmdID) {
    case OLECMDID_SETPROGRESSTEXT:
        CHECK_EXPECT(QueryStatus_SETPROGRESSTEXT);
        prgCmds[0].cmdf = OLECMDF_ENABLED;
        return S_OK;
    case OLECMDID_OPEN:
        CHECK_EXPECT(QueryStatus_OPEN);
        prgCmds[0].cmdf = 0;
        return S_OK;
    case OLECMDID_NEW:
        CHECK_EXPECT(QueryStatus_NEW);
        prgCmds[0].cmdf = 0;
        return S_OK;
    default:
        ok(0, "unexpected command %ld\n", prgCmds[0].cmdID);
    };

    return E_FAIL;
}

static HRESULT WINAPI OleCommandTarget_Exec(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    test_readyState(NULL);

    if(!pguidCmdGroup) {
        switch(nCmdID) {
        case OLECMDID_SETPROGRESSMAX:
            CHECK_EXPECT2(Exec_SETPROGRESSMAX);
            ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER, "nCmdexecopts=%08lx\n", nCmdexecopt);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            if(pvaIn) {
                ok(V_VT(pvaIn) == VT_I4, "V_VT(pvaIn)=%d, expected VT_I4\n", V_VT(pvaIn));
                if(load_state == LD_NO)
                    ok(V_I4(pvaIn) == 0, "V_I4(pvaIn)=%ld, expected 0\n", V_I4(pvaIn));
            }
            ok(pvaOut == NULL, "pvaOut=%p, expected NULL\n", pvaOut);
            return S_OK;
        case OLECMDID_SETPROGRESSPOS:
            CHECK_EXPECT2(Exec_SETPROGRESSPOS);
            ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER, "nCmdexecopts=%08lx\n", nCmdexecopt);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            if(pvaIn) {
                ok(V_VT(pvaIn) == VT_I4, "V_VT(pvaIn)=%d, expected VT_I4\n", V_VT(pvaIn));
                if(load_state == LD_NO)
                    ok(V_I4(pvaIn) == 0, "V_I4(pvaIn)=%ld, expected 0\n", V_I4(pvaIn));
            }
            ok(pvaOut == NULL, "pvaOut=%p, expected NULL\n", pvaOut);
            return S_OK;
        case OLECMDID_HTTPEQUIV_DONE:
            CHECK_EXPECT(Exec_HTTPEQUIV_DONE);
            ok(nCmdexecopt == 0, "nCmdexecopts=%08lx\n", nCmdexecopt);
            ok(pvaOut == NULL, "pvaOut=%p\n", pvaOut);
            ok(pvaIn == NULL, "pvaIn=%p\n", pvaIn);
            load_state = LD_COMPLETE;
            return S_OK;
        case OLECMDID_SETDOWNLOADSTATE:
            ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER, "nCmdexecopts=%08lx\n", nCmdexecopt);
            ok(pvaOut == NULL, "pvaOut=%p\n", pvaOut);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            ok(V_VT(pvaIn) == VT_I4, "V_VT(pvaIn)=%d\n", V_VT(pvaIn));

            switch(V_I4(pvaIn)) {
            case 0:
                CHECK_EXPECT(Exec_SETDOWNLOADSTATE_0);
                load_state = LD_INTERACTIVE;
                break;
            case 1:
                CHECK_EXPECT(Exec_SETDOWNLOADSTATE_1);
                break;
            default:
                ok(0, "unexpevted V_I4(pvaIn)=%ld\n", V_I4(pvaIn));
            }

            return S_OK;
        case OLECMDID_UPDATECOMMANDS:
            CHECK_EXPECT(Exec_UPDATECOMMANDS);
            ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER, "nCmdexecopts=%08lx\n", nCmdexecopt);
            ok(pvaIn == NULL, "pvaIn=%p\n", pvaIn);
            ok(pvaOut == NULL, "pvaOut=%p\n", pvaOut);
            return S_OK;
        case OLECMDID_SETTITLE:
            CHECK_EXPECT2(Exec_SETTITLE);
            ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER, "nCmdexecopts=%08lx\n", nCmdexecopt);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            ok(pvaOut == NULL, "pvaOut=%p\n", pvaOut);
            ok(V_VT(pvaIn) == VT_BSTR, "V_VT(pvaIn)=%d\n", V_VT(pvaIn));
            ok(V_BSTR(pvaIn) != NULL, "V_BSTR(pvaIn) == NULL\n"); /* TODO */
            return S_OK;
        case OLECMDID_HTTPEQUIV:
            CHECK_EXPECT2(Exec_HTTPEQUIV);
            ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER, "nCmdexecopts=%08lx\n", nCmdexecopt);
            /* TODO */
            return S_OK;
        default:
            ok(0, "unexpected command %ld\n", nCmdID);
            return E_FAIL;
        };
    }

    if(IsEqualGUID(&CGID_ShellDocView, pguidCmdGroup)) {
        ok(nCmdexecopt == 0, "nCmdexecopts=%08lx\n", nCmdexecopt);

        switch(nCmdID) {
        case 37:
            CHECK_EXPECT2(Exec_ShellDocView_37);
            ok(pvaOut == NULL, "pvaOut=%p, expected NULL\n", pvaOut);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            if(pvaIn) {
                ok(V_VT(pvaIn) == VT_I4, "V_VT(pvaIn)=%d, expected VT_I4\n", V_VT(pvaIn));
                ok(V_I4(pvaIn) == 0, "V_I4(pvaIn)=%ld, expected 0\n", V_I4(pvaIn));
            }
            return S_OK;
        default:
            ok(0, "unexpected command %ld\n", nCmdID);
            return E_FAIL;
        };
    }

    if(IsEqualGUID(&CGID_MSHTML, pguidCmdGroup)) {
        ok(nCmdexecopt == 0, "nCmdexecopts=%08lx\n", nCmdexecopt);

        switch(nCmdID) {
        case IDM_PARSECOMPLETE:
            CHECK_EXPECT(Exec_MSHTML_PARSECOMPLETE);
            ok(pvaIn == NULL, "pvaIn != NULL\n");
            ok(pvaOut == NULL, "pvaOut != NULL\n");
            return S_OK;
        default:
            ok(0, "unexpected command %ld\n", nCmdID);
        };
    }

    if(IsEqualGUID(&CGID_Undocumented, pguidCmdGroup))
        return E_FAIL; /* TODO */

    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IOleCommandTargetVtbl OleCommandTargetVtbl = {
    OleCommandTarget_QueryInterface,
    OleCommandTarget_AddRef,
    OleCommandTarget_Release,
    OleCommandTarget_QueryStatus,
    OleCommandTarget_Exec
};

static IOleCommandTarget OleCommandTarget = { &OleCommandTargetVtbl };

static HRESULT WINAPI Dispatch_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI Dispatch_AddRef(IDispatch *iface)
{
    return 2;
}

static ULONG WINAPI Dispatch_Release(IDispatch *iface)
{
    return 1;
}

static HRESULT WINAPI Dispatch_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_GetTypeInfo(IDispatch *iface, UINT iTInfo, LCID lcid,
        ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(IsEqualGUID(&IID_NULL, riid), "riid != IID_NULL\n");
    ok(pDispParams != NULL, "pDispParams == NULL\n");
    ok(pExcepInfo == NULL, "pExcepInfo=%p, expected NULL\n", pExcepInfo);
    ok(puArgErr != NULL, "puArgErr == NULL\n");
    ok(V_VT(pVarResult) == 0, "V_VT(pVarResult)=%d, expected 0\n", V_VT(pVarResult));
    ok(wFlags == DISPATCH_PROPERTYGET, "wFlags=%08x, expected DISPATCH_PROPERTYGET\n", wFlags);
    test_readyState(NULL);

    switch(dispIdMember) {
    case DISPID_AMBIENT_USERMODE:
        CHECK_EXPECT2(Invoke_AMBIENT_USERMODE);
        V_VT(pVarResult) = VT_BOOL;
        V_BOOL(pVarResult) = VARIANT_TRUE;
        return S_OK;
    case DISPID_AMBIENT_DLCONTROL:
        CHECK_EXPECT2(Invoke_AMBIENT_DLCONTROL);
        return E_FAIL;
    case DISPID_AMBIENT_OFFLINEIFNOTCONNECTED:
        CHECK_EXPECT2(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        return E_FAIL;
    case DISPID_AMBIENT_SILENT:
        CHECK_EXPECT2(Invoke_AMBIENT_SILENT);
        V_VT(pVarResult) = VT_BOOL;
        V_BOOL(pVarResult) = VARIANT_FALSE;
        return S_OK;
    case DISPID_AMBIENT_USERAGENT:
        CHECK_EXPECT(Invoke_AMBIENT_USERAGENT);
        return E_FAIL;
    case DISPID_AMBIENT_PALETTE:
        CHECK_EXPECT(Invoke_AMBIENT_PALETTE);
        return E_FAIL;
    };

    ok(0, "unexpected dispid %ld\n", dispIdMember);
    return E_FAIL;
}

static IDispatchVtbl DispatchVtbl = {
    Dispatch_QueryInterface,
    Dispatch_AddRef,
    Dispatch_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    Dispatch_Invoke
};

static IDispatch Dispatch = { &DispatchVtbl };

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface,
                                                     REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface, REFGUID guidService,
                                    REFIID riid, void **ppv)
{
    /*
     * Services used by HTMLDocument:
     *
     * IOleUndoManager
     * IInternetSecurityManager
     * ITargetFrame
     * {D5F78C80-5252-11CF-90FA-00AA0042106E}
     * HTMLFrameBase
     * IShellObject
     * {3050F312-98B5-11CF-BB82-00AA00BDCE0B}
     * {53A2D5B1-D2FC-11D0-84E0-006097C9987D}
     * {AD7F6C62-F6BD-11D2-959B-006097C553C8}
     * DefView (?)
     * {6D12FE80-7911-11CF-9534-0000C05BAE0B}
     * IElementBehaviorFactory
     * {3050F429-98B5-11CF-BB82-00AA00BDCE0B}
     * STopLevelBrowser
     * IHTMLWindow2
     * IInternetProtocol
     * IWebBrowserApp
     * UrlHostory
     * IHTMLEditHost
     * IHlinkFrame
     */

    if(IsEqualGUID(&IID_IHlinkFrame, guidService)) {
        ok(IsEqualGUID(&IID_IHlinkFrame, riid), "unexpected riid\n");
        *ppv = &HlinkFrame;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static IServiceProvider ServiceProvider = { &ServiceProviderVtbl };

static HRESULT QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IOleClientSite, riid))
        *ppv = &ClientSite;
    else if(IsEqualGUID(&IID_IOleDocumentSite, riid))
        *ppv = &DocumentSite;
    else if(IsEqualGUID(&IID_IDocHostUIHandler, riid) || IsEqualGUID(&IID_IDocHostUIHandler2, riid))
        *ppv = &DocHostUIHandler;
    else if(IsEqualGUID(&IID_IOleContainer, riid))
        *ppv = &OleContainer;
    else if(IsEqualGUID(&IID_IOleWindow, riid) || IsEqualGUID(&IID_IOleInPlaceSite, riid))
        *ppv = &InPlaceSite;
    else if(IsEqualGUID(&IID_IOleInPlaceUIWindow, riid) || IsEqualGUID(&IID_IOleInPlaceFrame, riid))
        *ppv = &InPlaceFrame;
    else if(IsEqualGUID(&IID_IOleCommandTarget , riid))
        *ppv = &OleCommandTarget;
    else if(IsEqualGUID(&IID_IDispatch, riid))
        *ppv = &Dispatch;
    else if(IsEqualGUID(&IID_IServiceProvider, riid))
        *ppv = &ServiceProvider;

    /* TODO:
     * IOleInPlaceSiteEx
     * {D48A6EC6-6A4A-11CF-94A7-444553540000}
     * {7BB0B520-B1A7-11D2-BB23-00C04F79ABCD}
     * {000670BA-0000-0000-C000-000000000046}
     */

    if(*ppv)
        return S_OK;
    return E_NOINTERFACE;
}

static LRESULT WINAPI wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void test_readyState(IUnknown *unk)
{
    IHTMLDocument2 *htmldoc;
    BSTR state;
    HRESULT hres;

    static IUnknown *_unk;

    static const WCHAR wszUninitialized[] = {'u','n','i','n','i','t','i','a','l','i','z','e','d',0};
    static const WCHAR wszLoading[] = {'l','o','a','d','i','n','g',0};
    static const WCHAR wszInteractive[] = {'i','n','t','e','r','a','c','t','i','v','e',0};
    static const WCHAR wszComplete[] = {'c','o','m','p','l','e','t','e',0};

    static const LPCWSTR expected_state[] = {
        wszUninitialized,
        wszLoading,
        wszInteractive,
        wszComplete,
        wszUninitialized
    };

    if(!unk) unk = _unk;
    else _unk = unk;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLDocument2, (void**)&htmldoc);
    ok(hres == S_OK, "QueryInterface(IID_IHTMLDocument2) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IHTMLDocument2_get_readyState(htmldoc, NULL);
    ok(hres == E_POINTER, "get_readyState failed: %08lx, expected \n", hres);

    hres = IHTMLDocument2_get_readyState(htmldoc, &state);
    ok(hres == S_OK, "get_ReadyState failed: %08lx\n", hres);
    ok(!lstrcmpW(state, expected_state[load_state]), "unexpected state, expected %d\n", load_state);

    IHTMLDocument_Release(htmldoc);
}

static void test_ConnectionPoint(IConnectionPointContainer *container, REFIID riid)
{
    IConnectionPointContainer *tmp_container = NULL;
    IConnectionPoint *cp;
    IID iid;
    HRESULT hres;

    hres = IConnectionPointContainer_FindConnectionPoint(container, riid, &cp);
    ok(hres == S_OK, "FindConnectionPoint failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IConnectionPoint_GetConnectionInterface(cp, &iid);
    ok(hres == S_OK, "GetConnectionInterface failed: %08lx\n", hres);
    ok(IsEqualGUID(riid, &iid), "wrong iid\n");

    hres = IConnectionPoint_GetConnectionInterface(cp, NULL);
    ok(hres == E_POINTER, "GetConnectionInterface failed: %08lx, expected E_POINTER\n", hres);

    hres = IConnectionPoint_GetConnectionPointContainer(cp, &tmp_container);
    ok(hres == S_OK, "GetConnectionPointContainer failed: %08lx\n", hres);
    ok(tmp_container == container, "container != tmp_container\n");
    if(SUCCEEDED(hres))
        IConnectionPointContainer_Release(tmp_container);

    hres = IConnectionPoint_GetConnectionPointContainer(cp, NULL);
    ok(hres == E_POINTER, "GetConnectionPointContainer failed: %08lx, expected E_POINTER\n", hres);

    if(IsEqualGUID(&IID_IPropertyNotifySink, riid)) {
        DWORD cookie;

        hres = IConnectionPoint_Advise(cp, (IUnknown*)&PropertyNotifySink, &cookie);
        ok(hres == S_OK, "Advise failed: %08lx\n", hres);
    }

    IConnectionPoint_Release(cp);
}

static void test_ConnectionPointContainer(IUnknown *unk)
{
    IConnectionPointContainer *container;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "QueryInterface(IID_IConnectionPointContainer) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    test_ConnectionPoint(container, &IID_IPropertyNotifySink);
    test_ConnectionPoint(container, &DIID_HTMLDocumentEvents);
    test_ConnectionPoint(container, &DIID_HTMLDocumentEvents2);

    IConnectionPointContainer_Release(container);
}

static void test_Load(IPersistMoniker *persist)
{
    IBindCtx *bind;
    HRESULT hres;

    test_readyState((IUnknown*)persist);

    CreateBindCtx(0, &bind);
    IBindCtx_RegisterObjectParam(bind, (LPOLESTR)SZ_HTML_CLIENTSITE_OBJECTPARAM,
                                 (IUnknown*)&ClientSite);

    SET_EXPECT(GetDisplayName);
    SET_EXPECT(GetHostInfo);
    SET_EXPECT(Invoke_AMBIENT_USERMODE);
    SET_EXPECT(Invoke_AMBIENT_SILENT);
    SET_EXPECT(Invoke_AMBIENT_DLCONTROL);
    SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    SET_EXPECT(Invoke_AMBIENT_USERAGENT);
    SET_EXPECT(Invoke_AMBIENT_PALETTE);
    SET_EXPECT(GetOptionKeyPath);
    SET_EXPECT(GetOverrideKeyPath);
    SET_EXPECT(GetWindow);
    SET_EXPECT(QueryStatus_SETPROGRESSTEXT);
    SET_EXPECT(Exec_SETPROGRESSMAX);
    SET_EXPECT(Exec_SETPROGRESSPOS);
    SET_EXPECT(Exec_ShellDocView_37);
    SET_EXPECT(OnChanged_READYSTATE);
    SET_EXPECT(BindToStorage);
    SET_EXPECT(GetContainer);
    SET_EXPECT(LockContainer);
    expect_LockContainer_fLock = TRUE;
    readystate_set_loading = TRUE;

    hres = IPersistMoniker_Load(persist, FALSE, &Moniker, bind, 0x12);
    ok(hres == S_OK, "Load failed: %08lx\n", hres);

    CHECK_CALLED(GetDisplayName);
    CHECK_CALLED(GetHostInfo);
    CHECK_CALLED(Invoke_AMBIENT_USERMODE);
    CHECK_CALLED(Invoke_AMBIENT_SILENT);
    CHECK_CALLED(Invoke_AMBIENT_DLCONTROL);
    CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    CHECK_CALLED(Invoke_AMBIENT_USERAGENT);
    CHECK_CALLED(Invoke_AMBIENT_PALETTE);
    CHECK_CALLED(GetOptionKeyPath);
    CHECK_CALLED(GetOverrideKeyPath);
    CHECK_CALLED(GetWindow);
    CHECK_CALLED(QueryStatus_SETPROGRESSTEXT);
    CHECK_CALLED(Exec_SETPROGRESSMAX);
    CHECK_CALLED(Exec_SETPROGRESSPOS);
    CHECK_CALLED(Exec_ShellDocView_37);
    CHECK_CALLED(OnChanged_READYSTATE);
    CHECK_CALLED(BindToStorage);
    CHECK_CALLED(GetContainer);
    CHECK_CALLED(LockContainer);

    set_clientsite = container_locked = TRUE;

    IBindCtx_Release(bind);

    test_readyState((IUnknown*)persist);
}

static void test_download(void)
{
    HWND hwnd;
    MSG msg;

    hwnd = FindWindowA("Internet Explorer_Hidden", NULL);
    ok(hwnd != NULL, "Could not find hidden window\n");

    test_readyState(NULL);

    SET_EXPECT(SetStatusText);
    SET_EXPECT(Exec_SETDOWNLOADSTATE_1);
    SET_EXPECT(GetDropTarget);
    SET_EXPECT(OnChanged_1005);
    SET_EXPECT(OnChanged_READYSTATE);
    SET_EXPECT(Exec_SETPROGRESSPOS);
    SET_EXPECT(Exec_SETDOWNLOADSTATE_0);
    SET_EXPECT(Exec_MSHTML_PARSECOMPLETE);
    SET_EXPECT(Exec_HTTPEQUIV_DONE);
    SET_EXPECT(Exec_SETTITLE);
    expect_status_text = (LPWSTR)0xdeadbeef; /* TODO */

    while(!called_Exec_HTTPEQUIV_DONE && GetMessage(&msg, hwnd, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CHECK_CALLED(SetStatusText);
    CHECK_CALLED(Exec_SETDOWNLOADSTATE_1);
    CHECK_CALLED(GetDropTarget);
    CHECK_CALLED(OnChanged_1005);
    CHECK_CALLED(OnChanged_READYSTATE);
    CHECK_CALLED(Exec_SETPROGRESSPOS);
    CHECK_CALLED(Exec_SETDOWNLOADSTATE_0);
    CHECK_CALLED(Exec_MSHTML_PARSECOMPLETE);
    CHECK_CALLED(Exec_HTTPEQUIV_DONE);
    CHECK_CALLED(Exec_SETTITLE);

    load_state = LD_COMPLETE;

    test_readyState(NULL);
}

static void test_Persist(IUnknown *unk)
{
    IPersistMoniker *persist_mon;
    IPersistFile *persist_file;
    GUID guid;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IPersistFile, (void**)&persist_file);
    ok(hres == S_OK, "QueryInterface(IID_IPersist) failed: %08lx\n", hres);
    if(SUCCEEDED(hres)) {
        hres = IPersist_GetClassID(persist_file, NULL);
        ok(hres == E_INVALIDARG, "GetClassID returned: %08lx, expected E_INVALIDARG\n", hres);

        hres = IPersist_GetClassID(persist_file, &guid);
        ok(hres == S_OK, "GetClassID failed: %08lx\n", hres);
        ok(IsEqualGUID(&CLSID_HTMLDocument, &guid), "guid != CLSID_HTMLDocument\n");

        IPersist_Release(persist_file);
    }

    hres = IUnknown_QueryInterface(unk, &IID_IPersistMoniker, (void**)&persist_mon);
    ok(hres == S_OK, "QueryInterface(IID_IPersistMoniker) failed: %08lx\n", hres);
    if(SUCCEEDED(hres)) {
        hres = IPersistMoniker_GetClassID(persist_mon, NULL);
        ok(hres == E_INVALIDARG, "GetClassID returned: %08lx, expected E_INVALIDARG\n", hres);

        hres = IPersistMoniker_GetClassID(persist_mon, &guid);
        ok(hres == S_OK, "GetClassID failed: %08lx\n", hres);
        ok(IsEqualGUID(&CLSID_HTMLDocument, &guid), "guid != CLSID_HTMLDocument\n");

        if(load_state == LD_DOLOAD)
            test_Load(persist_mon);

        test_readyState(unk);

        IPersistMoniker_Release(persist_mon);
    }
}

static const OLECMDF expect_cmds[OLECMDID_GETPRINTTEMPLATE+1] = {
    0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_OPEN */
    OLECMDF_SUPPORTED,                  /* OLECMDID_NEW */
    OLECMDF_SUPPORTED,                  /* OLECMDID_SAVE */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_SAVEAS */
    OLECMDF_SUPPORTED,                  /* OLECMDID_SAVECOPYAS */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_PRINT */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_PRINTPREVIEW */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_PAGESETUP */
    OLECMDF_SUPPORTED,                  /* OLECMDID_SPELL */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_PROPERTIES */
    OLECMDF_SUPPORTED,                  /* OLECMDID_CUT */
    OLECMDF_SUPPORTED,                  /* OLECMDID_COPY */
    OLECMDF_SUPPORTED,                  /* OLECMDID_PASTE */
    OLECMDF_SUPPORTED,                  /* OLECMDID_PASTESPECIAL */
    OLECMDF_SUPPORTED,                  /* OLECMDID_UNDO */
    OLECMDF_SUPPORTED,                  /* OLECMDID_REDO */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_SELECTALL */
    OLECMDF_SUPPORTED,                  /* OLECMDID_CLEARSELECTION */
    OLECMDF_SUPPORTED,                  /* OLECMDID_ZOOM */
    OLECMDF_SUPPORTED,                  /* OLECMDID_GETZOOMRANGE */
    0,
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_REFRESH */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_STOP */
    0,0,0,0,0,0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_STOPDOWNLOAD */
    0,0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_DELETE */
    0,0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_ENABLE_INTERACTION */
    OLECMDF_SUPPORTED,                  /* OLECMDID_ONUNLOAD */
    0,0,0,0,0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_SHOWPAGESETUP */
    OLECMDF_SUPPORTED,                  /* OLECMDID_SHOWPRINT */
    0,0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_CLOSE */
    0,0,0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_SETPRINTTEMPLATE */
    OLECMDF_SUPPORTED                   /* OLECMDID_GETPRINTTEMPLATE */
};

static void test_OleCommandTarget(IUnknown *unk)
{
    IOleCommandTarget *cmdtrg;
    OLECMD cmds[OLECMDID_GETPRINTTEMPLATE];
    int i;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleCommandTarget, (void**)&cmdtrg);
    ok(hres == S_OK, "QueryInterface(IIDIOleM=CommandTarget failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    for(i=0; i<OLECMDID_GETPRINTTEMPLATE; i++) {
        cmds[i].cmdID = i+1;
        cmds[i].cmdf = 0xf0f0;
    }

    SET_EXPECT(QueryStatus_OPEN);
    SET_EXPECT(QueryStatus_NEW);
    hres = IOleCommandTarget_QueryStatus(cmdtrg, NULL, sizeof(cmds)/sizeof(cmds[0]), cmds, NULL);
    ok(hres == S_OK, "QueryStatus failed: %08lx\n", hres);
    CHECK_CALLED(QueryStatus_OPEN);
    CHECK_CALLED(QueryStatus_NEW);

    for(i=0; i<OLECMDID_GETPRINTTEMPLATE; i++) {
        ok(cmds[i].cmdID == i+1, "cmds[%d].cmdID canged to %lx\n", i, cmds[i].cmdID);
        ok(cmds[i].cmdf == expect_cmds[i+1], "cmds[%d].cmdf=%lx, expected %x\n",
                i+1, cmds[i].cmdf, expect_cmds[i+1]);
    }

    IOleCommandTarget_Release(cmdtrg);
}

static void test_OleCommandTarget_fail(IUnknown *unk)
{
    IOleCommandTarget *cmdtrg;
    int i;
    HRESULT hres;

    OLECMD cmd[2] = {
        {OLECMDID_OPEN, 0xf0f0},
        {OLECMDID_GETPRINTTEMPLATE+1, 0xf0f0}
    };

    hres = IUnknown_QueryInterface(unk, &IID_IOleCommandTarget, (void**)&cmdtrg);
    ok(hres == S_OK, "QueryInterface(IIDIOleM=CommandTarget failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IOleCommandTarget_QueryStatus(cmdtrg, NULL, 0, NULL, NULL);
    ok(hres == S_OK, "QueryStatus failed: %08lx\n", hres);

    SET_EXPECT(QueryStatus_OPEN);
    hres = IOleCommandTarget_QueryStatus(cmdtrg, NULL, 2, cmd, NULL);
    CHECK_CALLED(QueryStatus_OPEN);

    ok(hres == OLECMDERR_E_NOTSUPPORTED,
            "QueryStatus failed: %08lx, expected OLECMDERR_E_NOTSUPPORTED\n", hres);
    ok(cmd[1].cmdID == OLECMDID_GETPRINTTEMPLATE+1,
            "cmd[0].cmdID=%ld, expected OLECMDID_GETPRINTTEMPLATE+1\n", cmd[0].cmdID);
    ok(cmd[1].cmdf == 0, "cmd[0].cmdf=%lx, expected 0\n", cmd[0].cmdf);
    ok(cmd[0].cmdf == OLECMDF_SUPPORTED,
            "cmd[1].cmdf=%lx, expected OLECMDF_SUPPORTED\n", cmd[1].cmdf);

    hres = IOleCommandTarget_QueryStatus(cmdtrg, &IID_IHTMLDocument2, 2, cmd, NULL);
    ok(hres == OLECMDERR_E_UNKNOWNGROUP,
            "QueryStatus failed: %08lx, expected OLECMDERR_E_UNKNOWNGROUP\n", hres);

    for(i=0; i<OLECMDID_GETPRINTTEMPLATE; i++) {
        if(!expect_cmds[i]) {
            hres = IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_UPDATECOMMANDS,
                    OLECMDEXECOPT_DODEFAULT, NULL, NULL);
            ok(hres == OLECMDERR_E_NOTSUPPORTED,
                    "Exec failed: %08lx, expected OLECMDERR_E_NOTSUPPORTED\n", hres);
        }
    }

    hres = IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_GETPRINTTEMPLATE+1,
            OLECMDEXECOPT_DODEFAULT, NULL, NULL);
    ok(hres == OLECMDERR_E_NOTSUPPORTED,
            "Exec failed: %08lx, expected OLECMDERR_E_NOTSUPPORTED\n", hres);

    IOleCommandTarget_Release(cmdtrg);
}

static void test_exec_onunload(IUnknown *unk)
{
    IOleCommandTarget *cmdtrg;
    VARIANT var;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleCommandTarget, (void**)&cmdtrg);
    ok(hres == S_OK, "QueryInterface(IID_IOleCommandTarget) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    memset(&var, 0x0a, sizeof(var));
    hres = IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_ONUNLOAD,
            OLECMDEXECOPT_DODEFAULT, NULL, &var);
    ok(hres == S_OK, "Exec(..., OLECMDID_ONUNLOAD, ...) failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_BOOL, "V_VT(var)=%d, expected VT_BOOL\n", V_VT(&var));
    ok(V_BOOL(&var) == VARIANT_TRUE, "V_BOOL(var)=%x, expected VARIANT_TRUE\n", V_BOOL(&var));

    hres = IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_ONUNLOAD,
            OLECMDEXECOPT_DODEFAULT, NULL, NULL);
    ok(hres == S_OK, "Exec(..., OLECMDID_ONUNLOAD, ...) failed: %08lx\n", hres);

    IOleCommandTarget_Release(cmdtrg);
}

static void test_exec_editmode(IUnknown *unk)
{
    IOleCommandTarget *cmdtrg;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleCommandTarget, (void**)&cmdtrg);
    ok(hres == S_OK, "QueryInterface(IID_IOleCommandTarget) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    SET_EXPECT(SetStatusText);
    SET_EXPECT(Exec_ShellDocView_37);
    SET_EXPECT(GetHostInfo);
    SET_EXPECT(Invoke_AMBIENT_SILENT);
    SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    SET_EXPECT(OnChanged_READYSTATE);
    expect_status_text = NULL;
    readystate_set_loading = TRUE;

    hres = IOleCommandTarget_Exec(cmdtrg, &CGID_MSHTML, IDM_EDITMODE,
            OLECMDEXECOPT_DODEFAULT, NULL, NULL);
    ok(hres == S_OK, "Exec failed: %08lx\n", hres);

    CHECK_CALLED(SetStatusText);
    CHECK_CALLED(Exec_ShellDocView_37);
    CHECK_CALLED(GetHostInfo);
    CHECK_CALLED(Invoke_AMBIENT_SILENT);
    CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    CHECK_CALLED(OnChanged_READYSTATE);

    IOleCommandTarget_Release(cmdtrg);
}

static HWND create_container_window(void)
{
    static const WCHAR wszHTMLDocumentTest[] =
        {'H','T','M','L','D','o','c','u','m','e','n','t','T','e','s','t',0};
    static WNDCLASSEXW wndclass = {
        sizeof(WNDCLASSEXW),
        0,
        wnd_proc,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        wszHTMLDocumentTest,
        NULL
    };

    RegisterClassExW(&wndclass);
    return CreateWindowW(wszHTMLDocumentTest, wszHTMLDocumentTest,
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, NULL, NULL, NULL, NULL);
}

static HRESULT test_DoVerb(IOleObject *oleobj)
{
    RECT rect = {0,0,500,500};
    HRESULT hres;

    if(!container_locked) {
        SET_EXPECT(GetContainer);
        SET_EXPECT(LockContainer);
    }
    SET_EXPECT(ActivateMe);
    expect_LockContainer_fLock = TRUE;

    hres = IOleObject_DoVerb(oleobj, OLEIVERB_SHOW, NULL, &ClientSite, -1, container_hwnd, &rect);
    if(FAILED(hres))
        return hres;
    ok(hres == S_OK, "DoVerb failed: %08lx\n", hres);

    if(!container_locked) {
        CHECK_CALLED(GetContainer);
        CHECK_CALLED(LockContainer);
        container_locked = TRUE;
    }
    CHECK_CALLED(ActivateMe);

    return hres;
}

#define CLIENTSITE_EXPECTPATH 0x00000001
#define CLIENTSITE_SETNULL    0x00000002
#define CLIENTSITE_DONTSET    0x00000004

static void test_ClientSite(IOleObject *oleobj, DWORD flags)
{
    IOleClientSite *clientsite;
    HRESULT hres;

    if(flags & CLIENTSITE_SETNULL) {
        hres = IOleObject_GetClientSite(oleobj, &clientsite);
        ok(clientsite == &ClientSite, "clientsite=%p, expected %p\n", clientsite, &ClientSite);

        hres = IOleObject_SetClientSite(oleobj, NULL);
        ok(hres == S_OK, "SetClientSite failed: %08lx\n", hres);

        set_clientsite = FALSE;
    }

    if(flags & CLIENTSITE_DONTSET)
        return;

    hres = IOleObject_GetClientSite(oleobj, &clientsite);
    ok(hres == S_OK, "GetClientSite failed: %08lx\n", hres);
    ok(clientsite == (set_clientsite ? &ClientSite : NULL), "GetClientSite() = %p, expected %p\n",
            clientsite, set_clientsite ? &ClientSite : NULL);

    if(!set_clientsite) {
        SET_EXPECT(GetHostInfo);
        if(flags & CLIENTSITE_EXPECTPATH) {
            SET_EXPECT(GetOptionKeyPath);
            SET_EXPECT(GetOverrideKeyPath);
        }
        SET_EXPECT(GetWindow);
        SET_EXPECT(QueryStatus_SETPROGRESSTEXT);
        SET_EXPECT(Exec_SETPROGRESSMAX);
        SET_EXPECT(Exec_SETPROGRESSPOS);
        SET_EXPECT(Invoke_AMBIENT_USERMODE);
        SET_EXPECT(Invoke_AMBIENT_DLCONTROL);
        SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        SET_EXPECT(Invoke_AMBIENT_SILENT);
        SET_EXPECT(Invoke_AMBIENT_USERAGENT);
        SET_EXPECT(Invoke_AMBIENT_PALETTE);

        hres = IOleObject_SetClientSite(oleobj, &ClientSite);
        ok(hres == S_OK, "SetClientSite failed: %08lx\n", hres);

        CHECK_CALLED(GetHostInfo);
        if(flags & CLIENTSITE_EXPECTPATH) {
            CHECK_CALLED(GetOptionKeyPath);
            CHECK_CALLED(GetOverrideKeyPath);
        }
        CHECK_CALLED(GetWindow);
        CHECK_CALLED(QueryStatus_SETPROGRESSTEXT);
        CHECK_CALLED(Exec_SETPROGRESSMAX);
        CHECK_CALLED(Exec_SETPROGRESSPOS);
        CHECK_CALLED(Invoke_AMBIENT_USERMODE);
        CHECK_CALLED(Invoke_AMBIENT_DLCONTROL);
        CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED); 
        CHECK_CALLED(Invoke_AMBIENT_SILENT);
        CHECK_CALLED(Invoke_AMBIENT_USERAGENT);
        CHECK_CALLED(Invoke_AMBIENT_PALETTE);

        set_clientsite = TRUE;
    }

    hres = IOleObject_SetClientSite(oleobj, &ClientSite);
    ok(hres == S_OK, "SetClientSite failed: %08lx\n", hres);

    hres = IOleObject_GetClientSite(oleobj, &clientsite);
    ok(hres == S_OK, "GetClientSite failed: %08lx\n", hres);
    ok(clientsite == &ClientSite, "GetClientSite() = %p, expected %p\n", clientsite, &ClientSite);
}

static void test_OnAmbientPropertyChange(IUnknown *unk)
{
    IOleControl *control = NULL;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleControl, (void**)&control);
    ok(hres == S_OK, "QueryInterface(IID_IOleControl failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    SET_EXPECT(Invoke_AMBIENT_USERMODE);
    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_USERMODE);
    ok(hres == S_OK, "OnAmbientChange failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_AMBIENT_USERMODE);

    SET_EXPECT(Invoke_AMBIENT_DLCONTROL);
    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_DLCONTROL);
    ok(hres == S_OK, "OnAmbientChange failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_AMBIENT_DLCONTROL);

    SET_EXPECT(Invoke_AMBIENT_DLCONTROL);
    SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_OFFLINEIFNOTCONNECTED);
    ok(hres == S_OK, "OnAmbientChange failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_AMBIENT_DLCONTROL);
    CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);

    SET_EXPECT(Invoke_AMBIENT_DLCONTROL);
    SET_EXPECT(Invoke_AMBIENT_SILENT);
    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_SILENT);
    ok(hres == S_OK, "OnAmbientChange failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_AMBIENT_DLCONTROL);
    CHECK_CALLED(Invoke_AMBIENT_SILENT);

    SET_EXPECT(Invoke_AMBIENT_USERAGENT);
    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_USERAGENT);
    ok(hres == S_OK, "OnAmbientChange failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_AMBIENT_USERAGENT);

    SET_EXPECT(Invoke_AMBIENT_PALETTE);
    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_PALETTE);
    ok(hres == S_OK, "OnAmbientChange failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_AMBIENT_PALETTE);

    IOleControl_Release(control);
}



static void test_OnAmbientPropertyChange2(IUnknown *unk)
{
    IOleControl *control = NULL;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleControl, (void**)&control);
    ok(hres == S_OK, "QueryInterface(IID_IOleControl failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_PALETTE);
    ok(hres == S_OK, "OnAmbientPropertyChange failed: %08lx\n", hres);

    IOleControl_Release(control);
}

static void test_Close(IUnknown *unk, BOOL set_client)
{
    IOleObject *oleobj = NULL;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_IOleObject) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    SET_EXPECT(GetContainer);
    SET_EXPECT(LockContainer);
    expect_LockContainer_fLock = FALSE;
    hres = IOleObject_Close(oleobj, OLECLOSE_NOSAVE);
    ok(hres == S_OK, "Close failed: %08lx\n", hres);
    CHECK_CALLED(GetContainer);
    CHECK_CALLED(LockContainer);
    container_locked = FALSE;

    if(set_client)
        test_ClientSite(oleobj, CLIENTSITE_SETNULL|CLIENTSITE_DONTSET);

    IOleObject_Release(oleobj);
}

static void test_OnFrameWindowActivate(IUnknown *unk)
{
    IOleInPlaceActiveObject *inplaceact;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleInPlaceActiveObject, (void**)&inplaceact);
    ok(hres == S_OK, "QueryInterface(IID_IOleInPlaceActiveObject) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    if(set_clientsite) {
        expect_OnFrameWindowActivate_fActivate = TRUE;
        SET_EXPECT(OnFrameWindowActivate);
        hres = IOleInPlaceActiveObject_OnFrameWindowActivate(inplaceact, TRUE);
        ok(hres == S_OK, "OnFrameWindowActivate failed: %08lx\n", hres);
        CHECK_CALLED(OnFrameWindowActivate);

        SET_EXPECT(OnFrameWindowActivate);
        hres = IOleInPlaceActiveObject_OnFrameWindowActivate(inplaceact, TRUE);
        ok(hres == S_OK, "OnFrameWindowActivate failed: %08lx\n", hres);
        CHECK_CALLED(OnFrameWindowActivate);

        expect_OnFrameWindowActivate_fActivate = FALSE;
        SET_EXPECT(OnFrameWindowActivate);
        hres = IOleInPlaceActiveObject_OnFrameWindowActivate(inplaceact, FALSE);
        ok(hres == S_OK, "OnFrameWindowActivate failed: %08lx\n", hres);
        CHECK_CALLED(OnFrameWindowActivate);

        expect_OnFrameWindowActivate_fActivate = TRUE;
        SET_EXPECT(OnFrameWindowActivate);
        hres = IOleInPlaceActiveObject_OnFrameWindowActivate(inplaceact, TRUE);
        ok(hres == S_OK, "OnFrameWindowActivate failed: %08lx\n", hres);
        CHECK_CALLED(OnFrameWindowActivate);
    }else {
        hres = IOleInPlaceActiveObject_OnFrameWindowActivate(inplaceact, FALSE);
        ok(hres == S_OK, "OnFrameWindowActivate failed: %08lx\n", hres);

        hres = IOleInPlaceActiveObject_OnFrameWindowActivate(inplaceact, TRUE);
        ok(hres == S_OK, "OnFrameWindowActivate failed: %08lx\n", hres);
    }

    IOleInPlaceActiveObject_Release(inplaceact);
}

static void test_InPlaceDeactivate(IUnknown *unk, BOOL expect_call)
{
    IOleInPlaceObjectWindowless *windowlessobj = NULL;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleInPlaceObjectWindowless,
            (void**)&windowlessobj);
    ok(hres == S_OK, "QueryInterface(IID_IOleInPlaceObjectWindowless) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    if(expect_call) SET_EXPECT(OnInPlaceDeactivate);
    hres = IOleInPlaceObjectWindowless_InPlaceDeactivate(windowlessobj);
    ok(hres == S_OK, "InPlaceDeactivate failed: %08lx\n", hres);
    if(expect_call) CHECK_CALLED(OnInPlaceDeactivate);

    IOleInPlaceObjectWindowless_Release(windowlessobj);
}

static HRESULT test_Activate(IUnknown *unk, DWORD flags)
{
    IOleObject *oleobj = NULL;
    GUID guid;
    HRESULT hres;

    last_hwnd = hwnd;

    if(view)
        IOleDocumentView_Release(view);
    view = NULL;

    hres = IUnknown_QueryInterface(unk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_IOleObject) failed: %08lx\n", hres);
    if(FAILED(hres))
        return hres;

    hres = IOleObject_GetUserClassID(oleobj, NULL);
    ok(hres == E_INVALIDARG, "GetUserClassID returned: %08lx, expected E_INVALIDARG\n", hres);

    hres = IOleObject_GetUserClassID(oleobj, &guid);
    ok(hres == S_OK, "GetUserClassID failed: %08lx\n", hres);
    ok(IsEqualGUID(&guid, &CLSID_HTMLDocument), "guid != CLSID_HTMLDocument\n");

    test_OnFrameWindowActivate(unk);

    test_ClientSite(oleobj, flags);
    test_InPlaceDeactivate(unk, FALSE);

    hres = test_DoVerb(oleobj);

    IOleObject_Release(oleobj);

    test_OnFrameWindowActivate(unk);

    return hres;
}

static void test_Window(IUnknown *unk, BOOL expect_success)
{
    IOleInPlaceActiveObject *activeobject = NULL;
    HWND tmp_hwnd;
    HRESULT hres;

    hres = IOleDocumentView_QueryInterface(view, &IID_IOleInPlaceActiveObject, (void**)&activeobject);
    ok(hres == S_OK, "Could not get IOleInPlaceActiveObject interface: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IOleInPlaceActiveObject_GetWindow(activeobject, &tmp_hwnd);

    if(expect_success) {
        ok(hres == S_OK, "GetWindow failed: %08lx\n", hres);
        ok(tmp_hwnd == hwnd, "tmp_hwnd=%p, expected %p\n", tmp_hwnd, hwnd);
    }else {
        ok(hres == E_FAIL, "GetWindow returned %08lx, expected E_FAIL\n", hres);
        ok(IsWindow(hwnd), "hwnd is destroyed\n");
    }

    IOleInPlaceActiveObject_Release(activeobject);
}

static void test_CloseView(void)
{
    IOleInPlaceSite *inplacesite = (IOleInPlaceSite*)0xff00ff00;
    HRESULT hres;

    if(!view)
        return;

    hres = IOleDocumentView_Show(view, FALSE);
    ok(hres == S_OK, "Show failed: %08lx\n", hres);

    hres = IOleDocumentView_CloseView(view, 0);
    ok(hres == S_OK, "CloseView failed: %08lx\n", hres);

    hres = IOleDocumentView_SetInPlaceSite(view, NULL);
    ok(hres == S_OK, "SetInPlaceSite failed: %08lx\n", hres);

    hres = IOleDocumentView_GetInPlaceSite(view, &inplacesite);
    ok(hres == S_OK, "SetInPlaceSite failed: %08lx\n", hres);
    ok(inplacesite == NULL, "inplacesite=%p, expected NULL\n", inplacesite);
}

static void test_UIDeactivate(void)
{
    HRESULT hres;

    if(call_UIActivate) {
        SET_EXPECT(SetActiveObject);
        SET_EXPECT(HideUI);
        SET_EXPECT(OnUIDeactivate);
    }

    expect_SetActiveObject_active = FALSE;
    hres = IOleDocumentView_UIActivate(view, FALSE);
    ok(hres == S_OK, "UIActivate failed: %08lx\n", hres);

    if(call_UIActivate) {
        CHECK_CALLED(SetActiveObject);
        CHECK_CALLED(HideUI);
        CHECK_CALLED(OnUIDeactivate);
    }
}

static void test_Hide(void)
{
    HRESULT hres;

    if(!view)
        return;

    hres = IOleDocumentView_Show(view, FALSE);
    ok(hres == S_OK, "Show failed: %08lx\n", hres);
}

static HRESULT create_document(IUnknown **unk)
{
    HRESULT hres = CoCreateInstance(&CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)unk);
    ok(hres == S_OK, "CoCreateInstance failed: %08lx\n", hres);
    return hres;
}

static void test_Navigate(IUnknown *unk)
{
    IHlinkTarget *hlink;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHlinkTarget, (void**)&hlink);
    ok(hres == S_OK, "QueryInterface(IID_IHlinkTarget) failed: %08lx\n", hres);

    SET_EXPECT(ActivateMe);
    hres = IHlinkTarget_Navigate(hlink, 0, NULL);
    ok(hres == S_OK, "Navigate failed: %08lx\n", hres);
    CHECK_CALLED(ActivateMe);

    IHlinkTarget_Release(hlink);
}

static void init_test(enum load_state_t ls) {
    hwnd = last_hwnd = NULL;
    set_clientsite = FALSE;
    call_UIActivate = FALSE;
    load_state = ls;
}

static void test_HTMLDocument(enum load_state_t ls)
{
    IUnknown *unk;
    HRESULT hres;
    ULONG ref;

    trace("Testing HTMLDocument (%s)...\n", (ls == LD_DOLOAD ? " load" : "no load"));

    init_test(ls);

    hres = create_document(&unk);
    if(FAILED(hres))
        return;

    test_ConnectionPointContainer(unk);
    test_Persist(unk);
    if(load_state == LD_NO)
        test_OnAmbientPropertyChange2(unk);

    hres = test_Activate(unk, CLIENTSITE_EXPECTPATH);
    if(FAILED(hres)) {
        IUnknown_Release(unk);
        return;
    }

    if(load_state == LD_LOADING)
        test_download();

    test_OleCommandTarget_fail(unk);
    test_OleCommandTarget(unk);
    test_OnAmbientPropertyChange(unk);
    test_Window(unk, TRUE);
    test_UIDeactivate();
    test_OleCommandTarget(unk);
    test_Window(unk, TRUE);
    test_InPlaceDeactivate(unk, TRUE);

    /* Calling test_OleCommandTarget here couses Segmentation Fault with native
     * MSHTML. It doesn't with Wine. */

    test_Window(unk, FALSE);
    test_Hide();
    test_InPlaceDeactivate(unk, FALSE);
    test_CloseView();
    test_Close(unk, FALSE);

    /* Activate HTMLDocument again */
    test_Activate(unk, CLIENTSITE_SETNULL);
    test_Window(unk, TRUE);
    test_OleCommandTarget(unk);
    test_UIDeactivate();
    test_InPlaceDeactivate(unk, TRUE);
    test_Close(unk, FALSE);

    /* Activate HTMLDocument again, this time without UIActivate */
    call_UIActivate = FALSE;
    test_Activate(unk, CLIENTSITE_SETNULL);
    test_Window(unk, TRUE);
    test_UIDeactivate();
    test_InPlaceDeactivate(unk, TRUE);
    test_CloseView();
    test_CloseView();
    test_Close(unk, TRUE);
    test_OnAmbientPropertyChange2(unk);

    if(view)
        IOleDocumentView_Release(view);
    view = NULL;

    ok(IsWindow(hwnd), "hwnd is destroyed\n");

    ref = IUnknown_Release(unk);
    ok(ref == 0, "ref=%ld, expected 0\n", ref);

    ok(!IsWindow(hwnd), "hwnd is not destroyed\n");
}

static void test_HTMLDocument_hlink(void)
{
    IUnknown *unk;
    HRESULT hres;
    ULONG ref;

    trace("Testing HTMLDocument (hlink)...\n");

    init_test(LD_DOLOAD);

    hres = create_document(&unk);
    if(FAILED(hres))
        return;

    test_ConnectionPointContainer(unk);
    test_Persist(unk);
    test_Navigate(unk);

    test_download();

    test_exec_onunload(unk);
    test_Window(unk, TRUE);
    test_InPlaceDeactivate(unk, TRUE);
    test_Close(unk, FALSE);

    if(view)
        IOleDocumentView_Release(view);
    view = NULL;

    ref = IUnknown_Release(unk);
    ok(ref == 0, "ref=%ld, expected 0\n", ref);
}

static void test_editing_mode(void)
{
    IUnknown *unk;
    IOleObject *oleobj;
    HRESULT hres;
    ULONG ref;

    trace("Testing HTMLDocument (edit)...\n");

    init_test(LD_DOLOAD);

    hres = create_document(&unk);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "Could not get IOleObject: %08lx\n", hres);

    test_readyState(unk);
    test_ConnectionPointContainer(unk);
    test_ClientSite(oleobj, CLIENTSITE_EXPECTPATH);
    test_DoVerb(oleobj);

    IOleObject_Release(oleobj);

    test_exec_editmode(unk);

    test_UIDeactivate();
    test_InPlaceDeactivate(unk, TRUE);
    test_Close(unk, FALSE);

    if(view) {
        IOleDocumentView_Release(view);
        view = NULL;
    }

    ref = IUnknown_Release(unk);
    ok(ref == 0, "ref=%ld, expected 0\n", ref);
}

static void gecko_installer_workaround(BOOL disable)
{
    HKEY hkey;
    DWORD res;

    static BOOL has_url = FALSE;
    static char url[2048];

    if(!disable && !has_url)
        return;

    res = RegOpenKey(HKEY_CURRENT_USER, "Software\\Wine\\MSHTML", &hkey);
    if(res != ERROR_SUCCESS)
        return;

    if(disable) {
        DWORD type, size = sizeof(url);

        res = RegQueryValueEx(hkey, "GeckoUrl", NULL, &type, (PVOID)url, &size);
        if(res == ERROR_SUCCESS && type == REG_SZ)
            has_url = TRUE;

        RegDeleteValue(hkey, "GeckoUrl");
    }else {
        RegSetValueEx(hkey, "GeckoUrl", 0, REG_SZ, (PVOID)url, lstrlenA(url)+1);
    }

    RegCloseKey(hkey);
}

START_TEST(htmldoc)
{
    gecko_installer_workaround(TRUE);

    CoInitialize(NULL);
    container_hwnd = create_container_window();

    test_HTMLDocument(LD_NO);
    test_HTMLDocument(LD_DOLOAD);
    test_HTMLDocument_hlink();
    test_editing_mode();

    DestroyWindow(container_hwnd);
    CoUninitialize();

    gecko_installer_workaround(FALSE);
}

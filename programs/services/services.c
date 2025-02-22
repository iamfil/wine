/*
 * Services - controls services keeps track of their state
 *
 * Copyright 2007 Google (Mikolaj Zalewski)
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

#define WIN32_LEAN_AND_MEAN

#include <stdarg.h>
#include <windows.h>
#include <winsvc.h>
#include <rpc.h>

#include "wine/unicode.h"
#include "wine/debug.h"
#include "svcctl.h"

#include "services.h"

#define MAX_SERVICE_NAME 260

WINE_DEFAULT_DEBUG_CHANNEL(service);

HANDLE g_hStartedEvent;
struct scmdatabase *active_database;

static const WCHAR SZ_LOCAL_SYSTEM[] = {'L','o','c','a','l','S','y','s','t','e','m',0};

/* Registry constants */
static const WCHAR SZ_SERVICES_KEY[] = { 'S','y','s','t','e','m','\\',
      'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
      'S','e','r','v','i','c','e','s',0 };

/* Service key values names */
static const WCHAR SZ_DISPLAY_NAME[]      = {'D','i','s','p','l','a','y','N','a','m','e',0 };
static const WCHAR SZ_TYPE[]              = {'T','y','p','e',0 };
static const WCHAR SZ_START[]             = {'S','t','a','r','t',0 };
static const WCHAR SZ_ERROR[]             = {'E','r','r','o','r','C','o','n','t','r','o','l',0 };
static const WCHAR SZ_IMAGE_PATH[]        = {'I','m','a','g','e','P','a','t','h',0};
static const WCHAR SZ_GROUP[]             = {'G','r','o','u','p',0};
static const WCHAR SZ_DEPEND_ON_SERVICE[] = {'D','e','p','e','n','d','O','n','S','e','r','v','i','c','e',0};
static const WCHAR SZ_DEPEND_ON_GROUP[]   = {'D','e','p','e','n','d','O','n','G','r','o','u','p',0};
static const WCHAR SZ_OBJECT_NAME[]       = {'O','b','j','e','c','t','N','a','m','e',0};
static const WCHAR SZ_TAG[]               = {'T','a','g',0};
static const WCHAR SZ_DESCRIPTION[]       = {'D','e','s','c','r','i','p','t','i','o','n',0};


DWORD service_create(LPCWSTR name, struct service_entry **entry)
{
    *entry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(**entry));
    if (!*entry)
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;
    (*entry)->name = strdupW(name);
    if (!(*entry)->name)
    {
        HeapFree(GetProcessHeap(), 0, *entry);
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;
    }
    (*entry)->control_pipe = INVALID_HANDLE_VALUE;
    return ERROR_SUCCESS;
}

void free_service_entry(struct service_entry *entry)
{
    HeapFree(GetProcessHeap(), 0, entry->name);
    HeapFree(GetProcessHeap(), 0, entry->config.lpBinaryPathName);
    HeapFree(GetProcessHeap(), 0, entry->config.lpDependencies);
    HeapFree(GetProcessHeap(), 0, entry->config.lpLoadOrderGroup);
    HeapFree(GetProcessHeap(), 0, entry->config.lpServiceStartName);
    HeapFree(GetProcessHeap(), 0, entry->config.lpDisplayName);
    HeapFree(GetProcessHeap(), 0, entry->description);
    HeapFree(GetProcessHeap(), 0, entry->dependOnServices);
    HeapFree(GetProcessHeap(), 0, entry->dependOnGroups);
    CloseHandle(entry->control_mutex);
    CloseHandle(entry->control_pipe);
    CloseHandle(entry->status_changed_event);
    HeapFree(GetProcessHeap(), 0, entry);
}

static DWORD load_service_config(HKEY hKey, struct service_entry *entry)
{
    DWORD err;
    WCHAR *wptr;

    if ((err = load_reg_string(hKey, SZ_IMAGE_PATH,   TRUE, &entry->config.lpBinaryPathName)) != 0)
        return err;
    if ((err = load_reg_string(hKey, SZ_GROUP,        0,    &entry->config.lpLoadOrderGroup)) != 0)
        return err;
    if ((err = load_reg_string(hKey, SZ_OBJECT_NAME,  TRUE, &entry->config.lpServiceStartName)) != 0)
        return err;
    if ((err = load_reg_string(hKey, SZ_DISPLAY_NAME, 0,    &entry->config.lpDisplayName)) != 0)
        return err;
    if ((err = load_reg_string(hKey, SZ_DESCRIPTION,  0,    &entry->description)) != 0)
        return err;
    if ((err = load_reg_multisz(hKey, SZ_DEPEND_ON_SERVICE, &entry->dependOnServices)) != 0)
        return err;
    if ((err = load_reg_multisz(hKey, SZ_DEPEND_ON_GROUP,   &entry->dependOnGroups)) != 0)
        return err;

    if ((err = load_reg_dword(hKey, SZ_TYPE,  &entry->config.dwServiceType)) != 0)
        return err;
    if ((err = load_reg_dword(hKey, SZ_START, &entry->config.dwStartType)) != 0)
        return err;
    if ((err = load_reg_dword(hKey, SZ_ERROR, &entry->config.dwErrorControl)) != 0)
        return err;
    if ((err = load_reg_dword(hKey, SZ_TAG,   &entry->config.dwTagId)) != 0)
        return err;

    WINE_TRACE("Image path           = %s\n", wine_dbgstr_w(entry->config.lpBinaryPathName) );
    WINE_TRACE("Group                = %s\n", wine_dbgstr_w(entry->config.lpLoadOrderGroup) );
    WINE_TRACE("Service account name = %s\n", wine_dbgstr_w(entry->config.lpServiceStartName) );
    WINE_TRACE("Display name         = %s\n", wine_dbgstr_w(entry->config.lpDisplayName) );
    WINE_TRACE("Service dependencies : %s\n", entry->dependOnServices[0] ? "" : "(none)");
    for (wptr = entry->dependOnServices; *wptr; wptr += strlenW(wptr) + 1)
        WINE_TRACE("    * %s\n", wine_dbgstr_w(wptr));
    WINE_TRACE("Group dependencies   : %s\n", entry->dependOnGroups[0] ? "" : "(none)");
    for (wptr = entry->dependOnGroups; *wptr; wptr += strlenW(wptr) + 1)
        WINE_TRACE("    * %s\n", wine_dbgstr_w(wptr));

    return ERROR_SUCCESS;
}

static DWORD reg_set_string_value(HKEY hKey, LPCWSTR value_name, LPCWSTR string)
{
    if (!string)
    {
        DWORD err;
        err = RegDeleteValueW(hKey, value_name);
        if (err != ERROR_FILE_NOT_FOUND)
            return err;

        return ERROR_SUCCESS;
    }

    return RegSetValueExW(hKey, value_name, 0, REG_SZ, (LPBYTE)string, sizeof(WCHAR)*(strlenW(string) + 1));
}

DWORD save_service_config(struct service_entry *entry)
{
    DWORD err;
    HKEY hKey = NULL;

    err = RegCreateKeyW(entry->db->root_key, entry->name, &hKey);
    if (err != ERROR_SUCCESS)
        goto cleanup;

    if ((err = reg_set_string_value(hKey, SZ_DISPLAY_NAME, entry->config.lpDisplayName)) != 0)
        goto cleanup;
    if ((err = reg_set_string_value(hKey, SZ_IMAGE_PATH, entry->config.lpBinaryPathName)) != 0)
        goto cleanup;
    if ((err = reg_set_string_value(hKey, SZ_GROUP, entry->config.lpLoadOrderGroup)) != 0)
        goto cleanup;
    if ((err = reg_set_string_value(hKey, SZ_OBJECT_NAME, entry->config.lpServiceStartName)) != 0)
        goto cleanup;
    if ((err = reg_set_string_value(hKey, SZ_DESCRIPTION, entry->description)) != 0)
        goto cleanup;
    if ((err = RegSetValueExW(hKey, SZ_START, 0, REG_DWORD, (LPBYTE)&entry->config.dwStartType, sizeof(DWORD))) != 0)
        goto cleanup;
    if ((err = RegSetValueExW(hKey, SZ_ERROR, 0, REG_DWORD, (LPBYTE)&entry->config.dwErrorControl, sizeof(DWORD))) != 0)
        goto cleanup;

    if ((err = RegSetValueExW(hKey, SZ_TYPE, 0, REG_DWORD, (LPBYTE)&entry->config.dwServiceType, sizeof(DWORD))) != 0)
        goto cleanup;

    if (entry->config.dwTagId)
        err = RegSetValueExW(hKey, SZ_TAG, 0, REG_DWORD, (LPBYTE)&entry->config.dwTagId, sizeof(DWORD));
    else
        err = RegDeleteValueW(hKey, SZ_TAG);

    if (err != 0 && err != ERROR_FILE_NOT_FOUND)
        goto cleanup;

    err = ERROR_SUCCESS;
cleanup:
    RegCloseKey(hKey);
    return err;
}

DWORD scmdatabase_add_service(struct scmdatabase *db, struct service_entry *service)
{
    int err;
    service->db = db;
    if ((err = save_service_config(service)) != ERROR_SUCCESS)
    {
        WINE_ERR("Couldn't store service configuration: error %u\n", err);
        return ERROR_GEN_FAILURE;
    }

    list_add_tail(&db->services, &service->entry);
    return ERROR_SUCCESS;
}

DWORD scmdatabase_remove_service(struct scmdatabase *db, struct service_entry *service)
{
    int err;

    err = RegDeleteTreeW(db->root_key, service->name);

    if (err != 0)
        return err;

    list_remove(&service->entry);
    service->entry.next = service->entry.prev = NULL;
    return ERROR_SUCCESS;
}

static void scmdatabase_autostart_services(struct scmdatabase *db)
{
    struct service_entry **services_list;
    unsigned int i = 0;
    unsigned int size = 32;
    struct service_entry *service;

    services_list = HeapAlloc(GetProcessHeap(), 0, size * sizeof(services_list[0]));
    if (!services_list)
        return;

    scmdatabase_lock_shared(db);

    LIST_FOR_EACH_ENTRY(service, &db->services, struct service_entry, entry)
    {
        if (service->config.dwStartType == SERVICE_BOOT_START ||
            service->config.dwStartType == SERVICE_SYSTEM_START ||
            service->config.dwStartType == SERVICE_AUTO_START)
        {
            if (i+1 >= size)
            {
                size *= 2;
                services_list = HeapReAlloc(GetProcessHeap(), 0, services_list, size * sizeof(services_list[0]));
                if (!services_list)
                    break;
            }
            services_list[i] = service;
            service->ref_count++;
            i++;
        }
    }

    scmdatabase_unlock(db);

    size = i;
    for (i = 0; i < size; i++)
    {
        DWORD err;
        service = services_list[i];
        err = service_start(service, 0, NULL);
        /* FIXME: do something if the service failed to start */
        release_service(service);
    }

    HeapFree(GetProcessHeap(), 0, services_list);
}

BOOL validate_service_name(LPCWSTR name)
{
    return (name && name[0] && !strchrW(name, '/') && !strchrW(name, '\\'));
}

BOOL validate_service_config(struct service_entry *entry)
{
    if (entry->config.dwServiceType & SERVICE_WIN32 && (entry->config.lpBinaryPathName == NULL || !entry->config.lpBinaryPathName[0]))
    {
        WINE_ERR("Service %s is Win32 but has no image path set\n", wine_dbgstr_w(entry->name));
        return FALSE;
    }

    switch (entry->config.dwServiceType)
    {
    case SERVICE_KERNEL_DRIVER:
    case SERVICE_FILE_SYSTEM_DRIVER:
    case SERVICE_WIN32_OWN_PROCESS:
    case SERVICE_WIN32_SHARE_PROCESS:
        /* No problem */
        break;
    case SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS:
    case SERVICE_WIN32_SHARE_PROCESS | SERVICE_INTERACTIVE_PROCESS:
        /* These can be only run as LocalSystem */
        if (entry->config.lpServiceStartName && strcmpiW(entry->config.lpServiceStartName, SZ_LOCAL_SYSTEM) != 0)
        {
            WINE_ERR("Service %s is interactive but has a start name\n", wine_dbgstr_w(entry->name));
            return FALSE;
        }
        break;
    default:
        WINE_ERR("Service %s has an unknown service type\n", wine_dbgstr_w(entry->name));
        return FALSE;
    }

    /* StartType can only be a single value (if several values are mixed the result is probably not what was intended) */
    if (entry->config.dwStartType > SERVICE_DISABLED)
    {
        WINE_ERR("Service %s has an unknown start type\n", wine_dbgstr_w(entry->name));
        return FALSE;
    }

    /* SERVICE_BOOT_START and SERVICE_SYSTEM_START are only allowed for driver services */
    if (((entry->config.dwStartType == SERVICE_BOOT_START) || (entry->config.dwStartType == SERVICE_SYSTEM_START)) &&
        ((entry->config.dwServiceType & SERVICE_WIN32_OWN_PROCESS) || (entry->config.dwServiceType & SERVICE_WIN32_SHARE_PROCESS)))
    {
        WINE_ERR("Service %s - SERVICE_BOOT_START and SERVICE_SYSTEM_START are only allowed for driver services\n", wine_dbgstr_w(entry->name));
        return FALSE;
    }

    if (entry->config.lpServiceStartName == NULL)
        entry->config.lpServiceStartName = strdupW(SZ_LOCAL_SYSTEM);

    return TRUE;
}


struct service_entry *scmdatabase_find_service(struct scmdatabase *db, LPCWSTR name)
{
    struct service_entry *service;

    LIST_FOR_EACH_ENTRY(service, &db->services, struct service_entry, entry)
    {
        if (strcmpiW(name, service->name) == 0)
            return service;
    }

    return NULL;
}

struct service_entry *scmdatabase_find_service_by_displayname(struct scmdatabase *db, LPCWSTR name)
{
    struct service_entry *service;

    LIST_FOR_EACH_ENTRY(service, &db->services, struct service_entry, entry)
    {
        if (strcmpiW(name, service->config.lpDisplayName) == 0)
            return service;
    }

    return NULL;
}

void release_service(struct service_entry *service)
{
    if (InterlockedDecrement(&service->ref_count) == 0 && is_marked_for_delete(service))
        free_service_entry(service);
}

static DWORD scmdatabase_create(struct scmdatabase **db)
{
    DWORD err;

    *db = HeapAlloc(GetProcessHeap(), 0, sizeof(**db));
    if (!*db)
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;

    (*db)->service_start_lock = FALSE;
    list_init(&(*db)->services);

    InitializeCriticalSection(&(*db)->cs);

    err = RegCreateKeyExW(HKEY_LOCAL_MACHINE, SZ_SERVICES_KEY, 0, NULL,
                          REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                          &(*db)->root_key, NULL);
    if (err != ERROR_SUCCESS)
        HeapFree(GetProcessHeap(), 0, *db);

    return err;
}

static void scmdatabase_destroy(struct scmdatabase *db)
{
    RegCloseKey(db->root_key);
    DeleteCriticalSection(&db->cs);
    HeapFree(GetProcessHeap(), 0, db);
}

static DWORD scmdatabase_load_services(struct scmdatabase *db)
{
    DWORD err;
    int i;

    for (i = 0; TRUE; i++)
    {
        WCHAR szName[MAX_SERVICE_NAME];
        struct service_entry *entry;
        HKEY hServiceKey;

        err = RegEnumKeyW(db->root_key, i, szName, MAX_SERVICE_NAME);
        if (err == ERROR_NO_MORE_ITEMS)
            break;

        if (err != 0)
        {
            WINE_ERR("Error %d reading key %d name - skipping\n", err, i);
            continue;
        }

        err = service_create(szName, &entry);
        if (err != ERROR_SUCCESS)
            break;

        WINE_TRACE("Loading service %s\n", wine_dbgstr_w(szName));
        err = RegOpenKeyExW(db->root_key, szName, 0, KEY_READ | KEY_WRITE, &hServiceKey);
        if (err == ERROR_SUCCESS)
        {
            err = load_service_config(hServiceKey, entry);
            RegCloseKey(hServiceKey);
        }

        if (err != ERROR_SUCCESS)
        {
            WINE_ERR("Error %d reading registry key for service %s - skipping\n", err, wine_dbgstr_w(szName));
            free_service_entry(entry);
            continue;
        }

        if (entry->config.dwServiceType == 0)
        {
            /* Maybe an application only wrote some configuration in the service key. Continue silently */
            WINE_TRACE("Even the service type not set for service %s - skipping\n", wine_dbgstr_w(szName));
            free_service_entry(entry);
            continue;
        }

        if (!validate_service_config(entry))
        {
            WINE_ERR("Invalid configuration of service %s - skipping\n", wine_dbgstr_w(szName));
            free_service_entry(entry);
            continue;
        }

        entry->status.dwServiceType = entry->config.dwServiceType;
        entry->status.dwCurrentState = SERVICE_STOPPED;
        entry->status.dwWin32ExitCode = ERROR_SERVICE_NEVER_STARTED;
        entry->db = db;
        /* all other fields are zero */

        list_add_tail(&db->services, &entry->entry);
    }
    return ERROR_SUCCESS;
}

DWORD scmdatabase_lock_startup(struct scmdatabase *db)
{
    if (InterlockedCompareExchange(&db->service_start_lock, TRUE, FALSE))
        return ERROR_SERVICE_DATABASE_LOCKED;
    return ERROR_SUCCESS;
}

void scmdatabase_unlock_startup(struct scmdatabase *db)
{
    InterlockedCompareExchange(&db->service_start_lock, FALSE, TRUE);
}

void scmdatabase_lock_shared(struct scmdatabase *db)
{
    EnterCriticalSection(&db->cs);
}

void scmdatabase_lock_exclusive(struct scmdatabase *db)
{
    EnterCriticalSection(&db->cs);
}

void scmdatabase_unlock(struct scmdatabase *db)
{
    LeaveCriticalSection(&db->cs);
}

void service_lock_shared(struct service_entry *service)
{
    EnterCriticalSection(&service->db->cs);
}

void service_lock_exclusive(struct service_entry *service)
{
    EnterCriticalSection(&service->db->cs);
}

void service_unlock(struct service_entry *service)
{
    LeaveCriticalSection(&service->db->cs);
}

/* only one service started at a time, so there is no race on the registry
 * value here */
static LPWSTR service_get_pipe_name(void)
{
    static const WCHAR format[] = { '\\','\\','.','\\','p','i','p','e','\\',
        'n','e','t','\\','N','t','C','o','n','t','r','o','l','P','i','p','e','%','u',0};
    static const WCHAR service_current_key_str[] = { 'S','Y','S','T','E','M','\\',
        'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
        'C','o','n','t','r','o','l','\\',
        'S','e','r','v','i','c','e','C','u','r','r','e','n','t',0};
    LPWSTR name;
    DWORD len;
    HKEY service_current_key;
    DWORD service_current = -1;
    LONG ret;
    DWORD type;

    ret = RegCreateKeyExW(HKEY_LOCAL_MACHINE, service_current_key_str, 0,
        NULL, REG_OPTION_VOLATILE, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL,
        &service_current_key, NULL);
    if (ret != ERROR_SUCCESS)
        return NULL;
    len = sizeof(service_current);
    ret = RegQueryValueExW(service_current_key, NULL, NULL, &type,
        (BYTE *)&service_current, &len);
    if ((ret == ERROR_SUCCESS && type == REG_DWORD) || ret == ERROR_FILE_NOT_FOUND)
    {
        service_current++;
        RegSetValueExW(service_current_key, NULL, 0, REG_DWORD,
            (BYTE *)&service_current, sizeof(service_current));
    }
    RegCloseKey(service_current_key);
    if ((ret != ERROR_SUCCESS || type != REG_DWORD) && (ret != ERROR_FILE_NOT_FOUND))
        return NULL;
    len = sizeof(format)/sizeof(WCHAR) + 10 /* strlenW("4294967295") */;
    name = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!name)
        return NULL;
    snprintfW(name, len, format, service_current);
    return name;
}

static DWORD service_start_process(struct service_entry *service_entry, HANDLE *process)
{
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    LPWSTR path = NULL;
    DWORD size;
    BOOL r;

    service_lock_exclusive(service_entry);

    if (service_entry->config.dwServiceType == SERVICE_KERNEL_DRIVER)
    {
        static const WCHAR winedeviceW[] = {'\\','w','i','n','e','d','e','v','i','c','e','.','e','x','e',' ',0};
        DWORD len = GetSystemDirectoryW( NULL, 0 ) + sizeof(winedeviceW)/sizeof(WCHAR) + strlenW(service_entry->name);

        if (!(path = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
            return ERROR_NOT_ENOUGH_SERVER_MEMORY;
        GetSystemDirectoryW( path, len );
        lstrcatW( path, winedeviceW );
        lstrcatW( path, service_entry->name );
    }
    else
    {
        size = ExpandEnvironmentStringsW(service_entry->config.lpBinaryPathName,NULL,0);
        path = HeapAlloc(GetProcessHeap(),0,size*sizeof(WCHAR));
        if (!path)
            return ERROR_NOT_ENOUGH_SERVER_MEMORY;
        ExpandEnvironmentStringsW(service_entry->config.lpBinaryPathName,path,size);
    }

    ZeroMemory(&si, sizeof(STARTUPINFOW));
    si.cb = sizeof(STARTUPINFOW);
    if (!(service_entry->config.dwServiceType & SERVICE_INTERACTIVE_PROCESS))
    {
        static WCHAR desktopW[] = {'_','_','w','i','n','e','s','e','r','v','i','c','e','_','w','i','n','s','t','a','t','i','o','n','\\','D','e','f','a','u','l','t',0};
        si.lpDesktop = desktopW;
    }

    service_entry->status.dwCurrentState = SERVICE_START_PENDING;
    service_entry->status.dwProcessId = pi.dwProcessId;

    service_unlock(service_entry);

    r = CreateProcessW(NULL, path, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    HeapFree(GetProcessHeap(),0,path);
    if (!r)
    {
        service_lock_exclusive(service_entry);
        service_entry->status.dwCurrentState = SERVICE_STOPPED;
        service_entry->status.dwProcessId = 0;
        service_unlock(service_entry);
        return GetLastError();
    }

    *process = pi.hProcess;
    CloseHandle( pi.hThread );

    return ERROR_SUCCESS;
}

static DWORD service_wait_for_startup(struct service_entry *service_entry, HANDLE process_handle)
{
    WINE_TRACE("%p\n", service_entry);

    for (;;)
    {
        DWORD dwCurrentStatus;
        HANDLE handles[2] = { service_entry->status_changed_event, process_handle };
        DWORD ret;
        ret = WaitForMultipleObjects(sizeof(handles)/sizeof(handles[0]), handles, FALSE, 20000);
        if (ret != WAIT_OBJECT_0)
            return ERROR_SERVICE_REQUEST_TIMEOUT;
        service_lock_shared(service_entry);
        dwCurrentStatus = service_entry->status.dwCurrentState;
        service_unlock(service_entry);
        if (dwCurrentStatus == SERVICE_RUNNING)
        {
            WINE_TRACE("Service started successfully\n");
            return ERROR_SUCCESS;
        }
        if (dwCurrentStatus != SERVICE_START_PENDING)
            return ERROR_SERVICE_REQUEST_TIMEOUT;
    }
}

/******************************************************************************
 * service_send_start_message
 */
static BOOL service_send_start_message(struct service_entry *service, LPCWSTR *argv, DWORD argc)
{
    DWORD i, len, count, result;
    service_start_info *ssi;
    LPWSTR p;
    BOOL r;

    WINE_TRACE("%s %p %d\n", wine_dbgstr_w(service->name), argv, argc);

    /* FIXME: this can block so should be done in another thread */
    r = ConnectNamedPipe(service->control_pipe, NULL);
    if (!r && GetLastError() != ERROR_PIPE_CONNECTED)
    {
        WINE_ERR("pipe connect failed\n");
        return FALSE;
    }

    /* calculate how much space do we need to send the startup info */
    len = strlenW(service->name) + 1;
    for (i=0; i<argc; i++)
        len += strlenW(argv[i])+1;
    len++;

    ssi = HeapAlloc(GetProcessHeap(),0,FIELD_OFFSET(service_start_info, data[len]));
    ssi->cmd = WINESERV_STARTINFO;
    ssi->control = 0;
    ssi->total_size = FIELD_OFFSET(service_start_info, data[len]);
    ssi->name_size = strlenW(service->name) + 1;
    strcpyW( ssi->data, service->name );

    /* copy service args into a single buffer*/
    p = &ssi->data[ssi->name_size];
    for (i=0; i<argc; i++)
    {
        strcpyW(p, argv[i]);
        p += strlenW(p) + 1;
    }
    *p=0;

    r = WriteFile(service->control_pipe, ssi, ssi->total_size, &count, NULL);
    if (r)
    {
        r = ReadFile(service->control_pipe, &result, sizeof result, &count, NULL);
        if (r && result)
        {
            SetLastError(result);
            r = FALSE;
        }
    }

    HeapFree(GetProcessHeap(),0,ssi);

    return r;
}

DWORD service_start(struct service_entry *service, DWORD service_argc, LPCWSTR *service_argv)
{
    DWORD err;
    LPWSTR name;
    HANDLE process_handle = NULL;

    err = scmdatabase_lock_startup(service->db);
    if (err != ERROR_SUCCESS)
        return err;

    if (service->control_pipe != INVALID_HANDLE_VALUE)
    {
        scmdatabase_unlock_startup(service->db);
        return ERROR_SERVICE_ALREADY_RUNNING;
    }

    service->control_mutex = CreateMutexW(NULL, TRUE, NULL);

    if (!service->status_changed_event)
        service->status_changed_event = CreateEventW(NULL, FALSE, FALSE, NULL);

    name = service_get_pipe_name();
    service->control_pipe = CreateNamedPipeW(name, PIPE_ACCESS_DUPLEX,
                  PIPE_TYPE_BYTE|PIPE_WAIT, 1, 256, 256, 10000, NULL );
    HeapFree(GetProcessHeap(), 0, name);
    if (service->control_pipe==INVALID_HANDLE_VALUE)
    {
        WINE_ERR("failed to create pipe for %s, error = %d\n",
            wine_dbgstr_w(service->name), GetLastError());
        scmdatabase_unlock_startup(service->db);
        return GetLastError();
    }

    err = service_start_process(service, &process_handle);

    if (err == ERROR_SUCCESS)
    {
        if (!service_send_start_message(service, service_argv, service_argc))
            err = ERROR_SERVICE_REQUEST_TIMEOUT;
    }

    if (err == ERROR_SUCCESS)
        err = service_wait_for_startup(service, process_handle);

    if (process_handle)
        CloseHandle(process_handle);

    ReleaseMutex(service->control_mutex);
    scmdatabase_unlock_startup(service->db);

    WINE_TRACE("returning %d\n", err);

    return err;
}


int main(int argc, char *argv[])
{
    static const WCHAR svcctl_started_event[] = SVCCTL_STARTED_EVENT;
    DWORD err;
    g_hStartedEvent = CreateEventW(NULL, TRUE, FALSE, svcctl_started_event);
    err = scmdatabase_create(&active_database);
    if (err != ERROR_SUCCESS)
        return err;
    if ((err = scmdatabase_load_services(active_database)) != ERROR_SUCCESS)
        return err;
    if ((err = RPC_Init()) == ERROR_SUCCESS)
    {
        scmdatabase_autostart_services(active_database);
        RPC_MainLoop();
    }
    scmdatabase_destroy(active_database);
    return err;
}

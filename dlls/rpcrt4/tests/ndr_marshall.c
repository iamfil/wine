/*
 * Unit test suite for ndr marshalling functions
 *
 * Copyright 2006 Huw Davies
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

#define NTDDI_WIN2K   0x05000000
#define NTDDI_VERSION NTDDI_WIN2K /* for some MIDL_STUB_MESSAGE fields */

#include "wine/test.h"
#include <windef.h>
#include <winbase.h>
#include <winnt.h>
#include <winerror.h>

#include "rpc.h"
#include "rpcdce.h"
#include "rpcproxy.h"


static int my_alloc_called;
static int my_free_called;
static void * CALLBACK my_alloc(size_t size)
{
    my_alloc_called++;
    return NdrOleAllocate(size);
}

static void CALLBACK my_free(void *ptr)
{
    my_free_called++;
    NdrOleFree(ptr);
}

static const MIDL_STUB_DESC Object_StubDesc = 
    {
    NULL,
    my_alloc,
    my_free,
    { 0 },
    0,
    0,
    0,
    0,
    NULL, /* format string, filled in by tests */
    1, /* -error bounds_check flag */
    0x20000, /* Ndr library version */
    0,
    0x50100a4, /* MIDL Version 5.1.164 */
    0,
    NULL,
    0,  /* notify & notify_flag routine table */
    1,  /* Flags */
    0,  /* Reserved3 */
    0,  /* Reserved4 */
    0   /* Reserved5 */
    };

static RPC_DISPATCH_FUNCTION IFoo_table[] =
{
    0
};

static RPC_DISPATCH_TABLE IFoo_v0_0_DispatchTable =
{
    0,
    IFoo_table
};

static const RPC_SERVER_INTERFACE IFoo___RpcServerInterface =
{
    sizeof(RPC_SERVER_INTERFACE),
    {{0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x12,0x34}},{0,0}},
    {{0x8a885d04,0x1ceb,0x11c9,{0x9f,0xe8,0x08,0x00,0x2b,0x10,0x48,0x60}},{2,0}},
    &IFoo_v0_0_DispatchTable,
    0,
    0,
    0,
    0,
    0,
};

static RPC_IF_HANDLE IFoo_v0_0_s_ifspec = (RPC_IF_HANDLE)& IFoo___RpcServerInterface;
static BOOL use_pointer_ids = FALSE;

static void determine_pointer_marshalling_style(void)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    char ch = 0xde;

    static const unsigned char fmtstr_up_char[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x2,            /* FC_CHAR */
        0x5c,           /* FC_PAD */
    };

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = NULL;

    NdrClientInitializeNew(
                           &RpcMessage,
                           &StubMsg,
                           &StubDesc,
                           0);

    StubMsg.BufferLength = 8;
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    NdrPointerMarshall(&StubMsg, (unsigned char*)&ch, fmtstr_up_char);
    ok(StubMsg.Buffer == StubMsg.BufferStart + 5, "%p %p\n", StubMsg.Buffer, StubMsg.BufferStart);

    use_pointer_ids = (*(unsigned int *)StubMsg.BufferStart != (unsigned int)&ch);
    trace("Pointer marshalling using %s\n", use_pointer_ids ? "pointer ids" : "pointer value");

    HeapFree(GetProcessHeap(), 0, StubMsg.BufferStart);
}

static void test_ndr_simple_type(void)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    long l, l2 = 0;

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = NULL;

    NdrClientInitializeNew(
                           &RpcMessage,
                           &StubMsg,
                           &StubDesc,
                           0);

    StubMsg.BufferLength = 16;
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    l = 0xcafebabe;
    NdrSimpleTypeMarshall(&StubMsg, (unsigned char*)&l, 8 /* FC_LONG */);
    ok(StubMsg.Buffer == StubMsg.BufferStart + 4, "%p %p\n", StubMsg.Buffer, StubMsg.BufferStart);
    ok(*(long*)StubMsg.BufferStart == l, "%ld\n", *(long*)StubMsg.BufferStart);

    StubMsg.Buffer = StubMsg.BufferStart + 1;
    NdrSimpleTypeMarshall(&StubMsg, (unsigned char*)&l, 8 /* FC_LONG */);
    ok(StubMsg.Buffer == StubMsg.BufferStart + 8, "%p %p\n", StubMsg.Buffer, StubMsg.BufferStart);
    ok(*(long*)(StubMsg.BufferStart + 4) == l, "%ld\n", *(long*)StubMsg.BufferStart);

    StubMsg.Buffer = StubMsg.BufferStart + 1;
    NdrSimpleTypeUnmarshall(&StubMsg, (unsigned char*)&l2, 8 /* FC_LONG */);
    ok(StubMsg.Buffer == StubMsg.BufferStart + 8, "%p %p\n", StubMsg.Buffer, StubMsg.BufferStart);
    ok(l2 == l, "%ld\n", l2);

    HeapFree(GetProcessHeap(), 0, StubMsg.BufferStart);
}

static void test_pointer_marshal(const unsigned char *formattypes,
                                 void *memsrc,
                                 long srcsize,
                                 const void *wiredata,
                                 ULONG wiredatalen,
                                 int(*cmp)(const void*,const void*,size_t),
                                 long num_additional_allocs,
                                 const char *msgpfx)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    DWORD size;
    void *ptr;
    unsigned char *mem, *mem_orig;

    my_alloc_called = my_free_called = 0;
    if(!cmp)
        cmp = memcmp;

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = formattypes;

    NdrClientInitializeNew(
                           &RpcMessage,
                           &StubMsg,
                           &StubDesc,
                           0);

    StubMsg.BufferLength = 0;
    NdrPointerBufferSize( &StubMsg,
                          memsrc,
                          formattypes );
    ok(StubMsg.BufferLength >= wiredatalen, "%s: length %d\n", msgpfx, StubMsg.BufferLength);

    /*NdrGetBuffer(&_StubMsg, _StubMsg.BufferLength, NULL);*/
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    StubMsg.BufferEnd = StubMsg.BufferStart + StubMsg.BufferLength;

    memset(StubMsg.BufferStart, 0x0, StubMsg.BufferLength); /* This is a hack to clear the padding between the ptr and longlong/double */

    ptr = NdrPointerMarshall( &StubMsg,  memsrc, formattypes );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
    ok(!memcmp(StubMsg.BufferStart, wiredata, wiredatalen), "%s: incorrectly marshaled\n", msgpfx);

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;

    if (0)
    {
    /* NdrPointerMemorySize crashes under Wine */
    size = NdrPointerMemorySize( &StubMsg, formattypes );
    ok(size == StubMsg.MemorySize, "%s: mem size %u size %u\n", msgpfx, StubMsg.MemorySize, size);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
    if(formattypes[1] & 0x10 /* FC_POINTER_DEREF */)
        ok(size == srcsize + 4, "%s: mem size %u\n", msgpfx, size);
    else
        ok(size == srcsize, "%s: mem size %u\n", msgpfx, size);

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 16;
    size = NdrPointerMemorySize( &StubMsg, formattypes );
    ok(size == StubMsg.MemorySize, "%s: mem size %u size %u\n", msgpfx, StubMsg.MemorySize, size);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
    if(formattypes[1] & 0x10 /* FC_POINTER_DEREF */)
        ok(size == srcsize + 4 + 16, "%s: mem size %u\n", msgpfx, size);
    else
        ok(size == srcsize + 16, "%s: mem size %u\n", msgpfx, size);

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 1;
    size = NdrPointerMemorySize( &StubMsg, formattypes );
    ok(size == StubMsg.MemorySize, "%s: mem size %u size %u\n", msgpfx, StubMsg.MemorySize, size);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
    if(formattypes[1] & 0x10 /* FC_POINTER_DEREF */)
        ok(size == srcsize + 4 + (srcsize == 8 ? 8 : 4), "%s: mem size %u\n", msgpfx, size);
    else
        ok(size == srcsize + (srcsize == 8 ? 8 : 4), "%s: mem size %u\n", msgpfx, size);
    }

    size = srcsize;
    if(formattypes[1] & 0x10) size += 4;

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;
    mem_orig = mem = HeapAlloc(GetProcessHeap(), 0, size); 

    if(formattypes[1] & 0x10 /* FC_POINTER_DEREF */)
        *(void**)mem = NULL;
    ptr = NdrPointerUnmarshall( &StubMsg, &mem, formattypes, 0 );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    ok(mem == mem_orig, "%s: mem has changed %p %p\n", msgpfx, mem, mem_orig);
    ok(!cmp(mem, memsrc, srcsize), "%s: incorrectly unmarshaled\n", msgpfx);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
    ok(StubMsg.MemorySize == 0, "%s: memorysize %d\n", msgpfx, StubMsg.MemorySize);
    ok(my_alloc_called == num_additional_allocs, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called); 
    my_alloc_called = 0;

    /* reset the buffer and call with must alloc */
    StubMsg.Buffer = StubMsg.BufferStart;
    if(formattypes[1] & 0x10 /* FC_POINTER_DEREF */)
        *(void**)mem = NULL;
    ptr = NdrPointerUnmarshall( &StubMsg, &mem, formattypes, 1 );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    /* doesn't allocate mem in this case */
todo_wine {
    ok(mem == mem_orig, "%s: mem has changed %p %p\n", msgpfx, mem, mem_orig);
 }
    ok(!cmp(mem, memsrc, srcsize), "%s: incorrectly unmarshaled\n", msgpfx);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
    ok(StubMsg.MemorySize == 0, "%s: memorysize %d\n", msgpfx, StubMsg.MemorySize);

todo_wine {
    ok(my_alloc_called == num_additional_allocs, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called); 
}
    my_alloc_called = 0;
    if(formattypes[0] != 0x11 /* FC_RP */)
    {
        /* now pass the address of a NULL ptr */
        mem = NULL;
        StubMsg.Buffer = StubMsg.BufferStart;
        ptr = NdrPointerUnmarshall( &StubMsg, &mem, formattypes, 0 );
        ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
        ok(mem != StubMsg.BufferStart + wiredatalen - srcsize, "%s: mem points to buffer %p %p\n", msgpfx, mem, StubMsg.BufferStart);
        ok(!cmp(mem, memsrc, size), "%s: incorrectly unmarshaled\n", msgpfx);
        ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
        ok(StubMsg.MemorySize == 0, "%s: memorysize %d\n", msgpfx, StubMsg.MemorySize);
        ok(my_alloc_called == num_additional_allocs + 1, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called); 
        my_alloc_called = 0;
        NdrPointerFree(&StubMsg, mem, formattypes);
 
        /* again pass address of NULL ptr, but pretend we're a server */
        mem = NULL;
        StubMsg.Buffer = StubMsg.BufferStart;
        StubMsg.IsClient = 0;
        ptr = NdrPointerUnmarshall( &StubMsg, &mem, formattypes, 0 );
        ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
        if (formattypes[2] == 0xd /* FC_ENUM16 */)
            ok(mem != StubMsg.BufferStart + wiredatalen - srcsize, "%s: mem points to buffer %p %p\n", msgpfx, mem, StubMsg.BufferStart);
        else
            ok(mem == StubMsg.BufferStart + wiredatalen - srcsize, "%s: mem doesn't point to buffer %p %p\n", msgpfx, mem, StubMsg.BufferStart);
        ok(!cmp(mem, memsrc, size), "%s: incorrectly unmarshaled\n", msgpfx);
        ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
        ok(StubMsg.MemorySize == 0, "%s: memorysize %d\n", msgpfx, StubMsg.MemorySize);
        if (formattypes[2] != 0xd /* FC_ENUM16 */) {
            ok(my_alloc_called == num_additional_allocs, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
            my_alloc_called = 0;
        }
    }
    HeapFree(GetProcessHeap(), 0, mem_orig);
    HeapFree(GetProcessHeap(), 0, StubMsg.BufferStart);
}

static int deref_cmp(const void *s1, const void *s2, size_t num)
{
    return memcmp(*(const void *const *)s1, *(const void *const *)s2, num);
}


static void test_simple_types(void)
{
    unsigned char wiredata[16];
    unsigned char ch;
    unsigned char *ch_ptr;
    unsigned short s;
    unsigned int i;
    unsigned long l;
    ULONGLONG ll;
    float f;
    double d;

    static const unsigned char fmtstr_up_char[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x2,            /* FC_CHAR */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_byte[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x1,            /* FC_BYTE */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_small[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x3,            /* FC_SMALL */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_usmall[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x4,            /* FC_USMALL */
        0x5c,           /* FC_PAD */
    };  
    static const unsigned char fmtstr_rp_char[] =
    {
        0x11, 0x8,      /* FC_RP [simple_pointer] */
        0x2,            /* FC_CHAR */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_rpup_char[] =
    {
        0x11, 0x14,     /* FC_RP [alloced_on_stack] */
        NdrFcShort( 0x2 ),      /* Offset= 2 (4) */
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x2,            /* FC_CHAR */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_rpup_char2[] =
    {
        0x11, 0x04,     /* FC_RP [alloced_on_stack] */
        NdrFcShort( 0x2 ),      /* Offset= 2 (4) */
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x2,            /* FC_CHAR */
        0x5c,           /* FC_PAD */
    };

    static const unsigned char fmtstr_up_wchar[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x5,            /* FC_WCHAR */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_short[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x6,            /* FC_SHORT */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_ushort[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x7,            /* FC_USHORT */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_enum16[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0xd,            /* FC_ENUM16 */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_long[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x8,            /* FC_LONG */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_ulong[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x9,            /* FC_ULONG */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_enum32[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0xe,            /* FC_ENUM32 */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_errorstatus[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x10,           /* FC_ERROR_STATUS_T */
        0x5c,           /* FC_PAD */
    };

    static const unsigned char fmtstr_up_longlong[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0xb,            /* FC_HYPER */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_float[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0xa,            /* FC_FLOAT */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_double[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0xc,            /* FC_DOUBLE */
        0x5c,           /* FC_PAD */
    };

    ch = 0xa5;
    ch_ptr = &ch;
    if (use_pointer_ids)
        *(unsigned int *)wiredata = 0x20000;
    else
        *(unsigned int *)wiredata = (unsigned int)ch_ptr;
    wiredata[4] = ch;
 
    test_pointer_marshal(fmtstr_up_char, ch_ptr, 1, wiredata, 5, NULL, 0, "up_char");
    test_pointer_marshal(fmtstr_up_byte, ch_ptr, 1, wiredata, 5, NULL, 0, "up_byte");
    test_pointer_marshal(fmtstr_up_small, ch_ptr, 1, wiredata, 5, NULL, 0,  "up_small");
    test_pointer_marshal(fmtstr_up_usmall, ch_ptr, 1, wiredata, 5, NULL, 0, "up_usmall");

    test_pointer_marshal(fmtstr_rp_char, ch_ptr, 1, &ch, 1, NULL, 0, "rp_char");

    test_pointer_marshal(fmtstr_rpup_char, &ch_ptr, 1, wiredata, 5, deref_cmp, 1, "rpup_char");
    test_pointer_marshal(fmtstr_rpup_char2, ch_ptr, 1, wiredata, 5, NULL, 0, "rpup_char2");

    s = 0xa597;
    if (use_pointer_ids)
        *(unsigned int *)wiredata = 0x20000;
    else
        *(unsigned int *)wiredata = (unsigned int)&s;
    *(unsigned short*)(wiredata + 4) = s;

    test_pointer_marshal(fmtstr_up_wchar, &s, 2, wiredata, 6, NULL, 0, "up_wchar");
    test_pointer_marshal(fmtstr_up_short, &s, 2, wiredata, 6, NULL, 0, "up_short");
    test_pointer_marshal(fmtstr_up_ushort, &s, 2, wiredata, 6, NULL, 0, "up_ushort");

    i = 0x7fff;
    if (use_pointer_ids)
        *(unsigned int *)wiredata = 0x20000;
    else
        *(unsigned int *)wiredata = (unsigned int)&i;
    *(unsigned short*)(wiredata + 4) = i;
    test_pointer_marshal(fmtstr_up_enum16, &i, 2, wiredata, 6, NULL, 0, "up_enum16");

    l = 0xcafebabe;
    if (use_pointer_ids)
        *(unsigned int *)wiredata = 0x20000;
    else
        *(unsigned int *)wiredata = (unsigned int)&l;
    *(unsigned long*)(wiredata + 4) = l;

    test_pointer_marshal(fmtstr_up_long, &l, 4, wiredata, 8, NULL, 0, "up_long");
    test_pointer_marshal(fmtstr_up_ulong, &l, 4, wiredata, 8, NULL, 0,  "up_ulong");
    test_pointer_marshal(fmtstr_up_enum32, &l, 4, wiredata, 8, NULL, 0,  "up_emun32");
    test_pointer_marshal(fmtstr_up_errorstatus, &l, 4, wiredata, 8, NULL, 0,  "up_errorstatus");

    ll = ((ULONGLONG)0xcafebabe) << 32 | 0xdeadbeef;
    if (use_pointer_ids)
        *(unsigned int *)wiredata = 0x20000;
    else
        *(unsigned int *)wiredata = (unsigned int)&ll;
    *(unsigned int **)(wiredata + 4) = 0;
    *(ULONGLONG*)(wiredata + 8) = ll;
    test_pointer_marshal(fmtstr_up_longlong, &ll, 8, wiredata, 16, NULL, 0, "up_longlong");

    f = 3.1415f;
    if (use_pointer_ids)
        *(unsigned int *)wiredata = 0x20000;
    else
        *(unsigned int *)wiredata = (unsigned int)&f;
    *(float*)(wiredata + 4) = f;
    test_pointer_marshal(fmtstr_up_float, &f, 4, wiredata, 8, NULL, 0, "up_float");

    d = 3.1415;
    if (use_pointer_ids)
        *(unsigned int *)wiredata = 0x20000;
    else
        *(unsigned int *)wiredata = (unsigned int)&d;
    *(unsigned int *)(wiredata + 4) = 0;
    *(double*)(wiredata + 8) = d;
    test_pointer_marshal(fmtstr_up_double, &d, 8, wiredata, 16, NULL, 0,  "up_double");

}

static void test_nontrivial_pointer_types(void)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    void *ptr;
    char **p1;
    char *p2;
    char ch;
    unsigned char *mem, *mem_orig;

    static const unsigned char fmtstr_ref_unique_out[] =
    {
        0x12, 0x8,	/* FC_UP [simple_pointer] */
        0x2,		/* FC_CHAR */
        0x5c,		/* FC_PAD */
        0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
        NdrFcShort( 0xfffffffa ),	/* Offset= -6 (0) */
    };

    p1 = &p2;
    p2 = &ch;
    ch = 0x22;

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = fmtstr_ref_unique_out;

    NdrClientInitializeNew(
                           &RpcMessage,
                           &StubMsg,
                           &StubDesc,
                           0);

    StubMsg.BufferLength = 0;
    NdrPointerBufferSize( &StubMsg,
                          (unsigned char *)p1,
                          &fmtstr_ref_unique_out[4] );

    /* Windows overestimates the buffer size */
    ok(StubMsg.BufferLength >= 5, "length %d\n", StubMsg.BufferLength);

    /*NdrGetBuffer(&_StubMsg, _StubMsg.BufferLength, NULL);*/
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    StubMsg.BufferEnd = StubMsg.BufferStart + StubMsg.BufferLength;

    ptr = NdrPointerMarshall( &StubMsg, (unsigned char *)p1, &fmtstr_ref_unique_out[4] );
    ok(ptr == NULL, "ret %p\n", ptr);
    ok(StubMsg.Buffer - StubMsg.BufferStart == 5, "Buffer %p Start %p len %d\n",
       StubMsg.Buffer, StubMsg.BufferStart, StubMsg.Buffer - StubMsg.BufferStart);
    ok(*(unsigned int *)StubMsg.BufferStart != 0, "pointer ID marshalled incorrectly\n");
    ok(*(unsigned char *)(StubMsg.BufferStart + 4) == 0x22, "char data marshalled incorrectly: 0x%x\n",
       *(unsigned char *)(StubMsg.BufferStart + 4));

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;
    mem = NULL;

    /* Client */
    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    mem = mem_orig = HeapAlloc(GetProcessHeap(), 0, sizeof(void *));
    *(void **)mem = NULL;
    NdrPointerUnmarshall( &StubMsg, &mem, &fmtstr_ref_unique_out[4], 0);
    ok(mem == mem_orig, "mem alloced\n");
    ok(my_alloc_called == 1, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, &fmtstr_ref_unique_out[4], 1);
    todo_wine {
        ok(mem == mem_orig, "mem alloced\n");
        ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);
    }

    my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerFree( &StubMsg, mem, &fmtstr_ref_unique_out[4] );
    ok(my_free_called == 1, "free called %d\n", my_free_called);

    mem = my_alloc(sizeof(void *));
    *(void **)mem = NULL;
    my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerFree( &StubMsg, mem, &fmtstr_ref_unique_out[4] );
    ok(my_free_called == 0, "free called %d\n", my_free_called);
    my_free(mem);

    mem = my_alloc(sizeof(void *));
    *(void **)mem = my_alloc(sizeof(char));
    my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerFree( &StubMsg, mem, &fmtstr_ref_unique_out[4] );
    ok(my_free_called == 1, "free called %d\n", my_free_called);
    my_free(mem);

    /* Server */
    my_alloc_called = 0;
    StubMsg.IsClient = 0;
    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, &fmtstr_ref_unique_out[4], 0);
    ok(mem != StubMsg.BufferStart, "mem pointing at buffer\n");
    todo_wine
    ok(my_alloc_called == 1, "alloc called %d\n", my_alloc_called);
    NdrPointerFree( &StubMsg, mem, &fmtstr_ref_unique_out[4] );

    my_alloc_called = 0;
    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, &fmtstr_ref_unique_out[4], 1);
    ok(mem != StubMsg.BufferStart, "mem pointing at buffer\n");
    todo_wine
    ok(my_alloc_called == 1, "alloc called %d\n", my_alloc_called);
    NdrPointerFree( &StubMsg, mem, &fmtstr_ref_unique_out[4] );

    my_alloc_called = 0;
    mem = mem_orig;
    *(void **)mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, &fmtstr_ref_unique_out[4], 0);
    todo_wine {
        ok(mem == mem_orig, "mem alloced\n");
        ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);
    }

    my_alloc_called = 0;
    mem = mem_orig;
    *(void **)mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, &fmtstr_ref_unique_out[4], 1);
    todo_wine {
        ok(mem == mem_orig, "mem alloced\n");
        ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);
    }

    mem = my_alloc(sizeof(void *));
    *(void **)mem = NULL;
    my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerFree( &StubMsg, mem, &fmtstr_ref_unique_out[4] );
    ok(my_free_called == 0, "free called %d\n", my_free_called);
    my_free(mem);

    mem = my_alloc(sizeof(void *));
    *(void **)mem = my_alloc(sizeof(char));
    my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerFree( &StubMsg, mem, &fmtstr_ref_unique_out[4] );
    ok(my_free_called == 1, "free called %d\n", my_free_called);
    my_free(mem);

    HeapFree(GetProcessHeap(), 0, mem_orig);
    HeapFree(GetProcessHeap(), 0, StubMsg.RpcMsg->Buffer);
}

static void test_simple_struct_marshal(const unsigned char *formattypes,
                                       void *memsrc,
                                       long srcsize,
                                       const void *wiredata,
                                       ULONG wiredatalen,
                                       int(*cmp)(const void*,const void*,size_t),
                                       long num_additional_allocs,
                                       const char *msgpfx)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    DWORD size;
    void *ptr;
    unsigned char *mem, *mem_orig;

    my_alloc_called = my_free_called = 0;
    if(!cmp)
        cmp = memcmp;

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = formattypes;

    NdrClientInitializeNew(&RpcMessage, &StubMsg, &StubDesc, 0);

    StubMsg.BufferLength = 0;
    NdrSimpleStructBufferSize( &StubMsg, (unsigned char *)memsrc, formattypes );
    ok(StubMsg.BufferLength >= wiredatalen, "%s: length %d\n", msgpfx, StubMsg.BufferLength);
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    StubMsg.BufferEnd = StubMsg.BufferStart + StubMsg.BufferLength;
    ptr = NdrSimpleStructMarshall( &StubMsg,  (unsigned char*)memsrc, formattypes );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart);
    ok(!memcmp(StubMsg.BufferStart, wiredata, wiredatalen), "%s: incorrectly marshaled %08x %08x %08x\n", msgpfx, *(DWORD*)StubMsg.BufferStart,*((DWORD*)StubMsg.BufferStart+1),*((DWORD*)StubMsg.BufferStart+2));

    if (0)
    {
    /* FIXME: Causes Wine to crash */
    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;
    size = NdrSimpleStructMemorySize( &StubMsg, formattypes );
    ok(size == StubMsg.MemorySize, "%s: size != MemorySize\n", msgpfx);
    ok(size == srcsize, "%s: mem size %u\n", msgpfx, size);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart);

    StubMsg.Buffer = StubMsg.BufferStart;
    size = NdrSimpleStructMemorySize( &StubMsg, formattypes );
todo_wine {
    ok(size == StubMsg.MemorySize, "%s: size != MemorySize\n", msgpfx);
}
    ok(StubMsg.MemorySize == ((srcsize + 3) & ~3) + srcsize, "%s: mem size %u\n", msgpfx, size);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart);
    }
    size = srcsize;
    /*** Unmarshalling first with must_alloc false ***/

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;
    mem_orig = mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, srcsize);
    ptr = NdrSimpleStructUnmarshall( &StubMsg, &mem, formattypes, 0 );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart);
    ok(mem == mem_orig, "%s: mem has changed %p %p\n", msgpfx, mem, mem_orig);
    ok(!cmp(mem, memsrc, srcsize), "%s: incorrectly unmarshaled\n", msgpfx);
    ok(my_alloc_called == num_additional_allocs, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called); 
    my_alloc_called = 0;
    ok(StubMsg.MemorySize == 0, "%s: memorysize touched in unmarshal\n", msgpfx);

    /* If we're a server we still use the supplied memory */
    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.IsClient = 0;
    ptr = NdrSimpleStructUnmarshall( &StubMsg, &mem, formattypes, 0 );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    ok(mem == mem_orig, "%s: mem has changed %p %p\n", msgpfx, mem, mem_orig);
    ok(!cmp(mem, memsrc, srcsize), "%s: incorrectly unmarshaled\n", msgpfx); 
    ok(my_alloc_called == num_additional_allocs, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    my_alloc_called = 0;
    ok(StubMsg.MemorySize == 0, "%s: memorysize touched in unmarshal\n", msgpfx);

    /* ...unless we pass a NULL ptr, then the buffer is used. 
       Passing a NULL ptr while we're a client && !must_alloc
       crashes on Windows, so we won't do that. */

    mem = NULL;
    StubMsg.IsClient = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    ptr = NdrSimpleStructUnmarshall( &StubMsg, &mem, formattypes, 0 );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    ok(mem == StubMsg.BufferStart, "%s: mem not equal buffer\n", msgpfx);
    ok(!cmp(mem, memsrc, srcsize), "%s: incorrectly unmarshaled\n", msgpfx);
    ok(my_alloc_called == num_additional_allocs, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    my_alloc_called = 0;
    ok(StubMsg.MemorySize == 0, "%s: memorysize touched in unmarshal\n", msgpfx);

    /*** now must_alloc is true ***/

    /* with must_alloc set we always allocate new memory whether or not we're
       a server and also when passing NULL */
    mem = mem_orig;
    StubMsg.IsClient = 1;
    StubMsg.Buffer = StubMsg.BufferStart;
    ptr = NdrSimpleStructUnmarshall( &StubMsg, &mem, formattypes, 1 );
    ok(ptr == NULL, "ret %p\n", ptr);
    ok(mem != mem_orig, "mem not changed %p %p\n", mem, mem_orig);
    ok(!cmp(mem, memsrc, srcsize), "incorrectly unmarshaled\n");
    ok(my_alloc_called == num_additional_allocs + 1, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    my_alloc_called = 0;
    ok(StubMsg.MemorySize == 0, "memorysize touched in unmarshal\n");

    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    ptr = NdrSimpleStructUnmarshall( &StubMsg, &mem, formattypes, 1 );
    ok(ptr == NULL, "ret %p\n", ptr);
    ok(mem != mem_orig, "mem not changed %p %p\n", mem, mem_orig);
    ok(!cmp(mem, memsrc, srcsize), "incorrectly unmarshaled\n");
    ok(my_alloc_called == num_additional_allocs + 1, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    my_alloc_called = 0; 
    ok(StubMsg.MemorySize == 0, "memorysize touched in unmarshal\n");

    mem = mem_orig;
    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.IsClient = 0;
    StubMsg.ReuseBuffer = 1;
    ptr = NdrSimpleStructUnmarshall( &StubMsg, &mem, formattypes, 1 );
    ok(ptr == NULL, "ret %p\n", ptr);
    ok(mem != mem_orig, "mem not changed %p %p\n", mem, mem_orig);
    ok(mem != StubMsg.BufferStart, "mem is buffer mem\n");
    ok(!cmp(mem, memsrc, srcsize), "incorrectly unmarshaled\n");
    ok(my_alloc_called == num_additional_allocs + 1, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    my_alloc_called = 0;
    ok(StubMsg.MemorySize == 0, "memorysize touched in unmarshal\n");

    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.IsClient = 0;
    StubMsg.ReuseBuffer = 1;
    ptr = NdrSimpleStructUnmarshall( &StubMsg, &mem, formattypes, 1 );
    ok(ptr == NULL, "ret %p\n", ptr);
    ok(mem != StubMsg.BufferStart, "mem is buffer mem\n");
    ok(!cmp(mem, memsrc, srcsize), "incorrectly unmarshaled\n"); 
    ok(my_alloc_called == num_additional_allocs + 1, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    my_alloc_called = 0;
    ok(StubMsg.MemorySize == 0, "memorysize touched in unmarshal\n");

    HeapFree(GetProcessHeap(), 0, mem_orig);
    HeapFree(GetProcessHeap(), 0, StubMsg.BufferStart);
}

typedef struct
{
    long l1;
    long *pl1;
    char *pc1;
} ps1_t;

static int ps1_cmp(const void *s1, const void *s2, size_t num)
{
    const ps1_t *p1, *p2;

    p1 = s1;
    p2 = s2;

    if(p1->l1 != p2->l1)
        return 1;

    if(p1->pl1 && p2->pl1)
    {
        if(*p1->pl1 != *p2->pl1)
            return 1;
    }
    else if(p1->pl1 || p1->pl1)
        return 1;

    if(p1->pc1 && p2->pc1)
    {
        if(*p1->pc1 != *p2->pc1)
            return 1;
    }
    else if(p1->pc1 || p1->pc1)
        return 1;

    return 0;
}

static void test_simple_struct(void)
{
    unsigned char wiredata[28];
    unsigned long wiredatalen;
    long l;
    char c;
    ps1_t ps1;

    static const unsigned char fmtstr_simple_struct[] =
    {
        0x12, 0x0,      /* FC_UP */
        NdrFcShort( 0x2 ), /* Offset=2 */
        0x15, 0x3,      /* FC_STRUCT [align 4] */
        NdrFcShort( 0x18 ),      /* [size 24] */
        0x6,            /* FC_SHORT */
        0x2,            /* FC_CHAR */ 
        0x38,		/* FC_ALIGNM4 */
	0x8,		/* FC_LONG */
	0x8,		/* FC_LONG */
        0x39,		/* FC_ALIGNM8 */
        0xb,		/* FC_HYPER */ 
        0x5b,		/* FC_END */
    };
    struct {
        short s;
        char c;
        long l1, l2;
        LONGLONG ll;
    } s1;

    static const unsigned char fmtstr_pointer_struct[] =
    { 
        0x12, 0x0,      /* FC_UP */
        NdrFcShort( 0x2 ), /* Offset=2 */
        0x16, 0x3,      /* FC_PSTRUCT [align 4] */
        NdrFcShort( 0xc ),      /* [size 12] */
        0x4b,		/* FC_PP */
        0x5c,		/* FC_PAD */
        0x46,		/* FC_NO_REPEAT */
        0x5c,		/* FC_PAD */
        NdrFcShort( 0x4 ),	/* 4 */
	NdrFcShort( 0x4 ),	/* 4 */
        0x13, 0x8,	/* FC_OP [simple_pointer] */
        0x8,		/* FC_LONG */
        0x5c,		/* FC_PAD */
        0x46,		/* FC_NO_REPEAT */
        0x5c,		/* FC_PAD */
	NdrFcShort( 0x8 ),	/* 8 */
	NdrFcShort( 0x8 ),	/* 8 */
	0x13, 0x8,	/* FC_OP [simple_pointer] */
        0x2,		/* FC_CHAR */
        0x5c,		/* FC_PAD */
        0x5b,		/* FC_END */
        0x8,		/* FC_LONG */
        0x8,		/* FC_LONG */
        0x8,		/* FC_LONG */
        0x5c,		/* FC_PAD */
        0x5b,		/* FC_END */

    };

    /* zero the entire structure, including the holes */
    memset(&s1, 0, sizeof(s1));

    /* FC_STRUCT */
    s1.s = 0x1234;
    s1.c = 0xa5;
    s1.l1 = 0xdeadbeef;
    s1.l2 = 0xcafebabe;
    s1.ll = ((LONGLONG) 0xbadefeed << 32) | 0x2468ace0;

    wiredatalen = 24;
    memcpy(wiredata, &s1, wiredatalen); 
    test_simple_struct_marshal(fmtstr_simple_struct + 4, &s1, 24, wiredata, 24, NULL, 0, "struct");

    if (use_pointer_ids)
        *(unsigned int *)wiredata = 0x20000;
    else
        *(unsigned int *)wiredata = (unsigned int)&s1;
    memcpy(wiredata + 4, &s1, wiredatalen);
    if (0)
    {
    /* one of the unmarshallings crashes Wine */
    test_pointer_marshal(fmtstr_simple_struct, &s1, 24, wiredata, 28, NULL, 0, "struct");
    }

    /* zero the entire structure, including the hole */
    memset(&ps1, 0, sizeof(ps1));

    /* FC_PSTRUCT */
    ps1.l1 = 0xdeadbeef;
    l = 0xcafebabe;
    ps1.pl1 = &l;
    c = 'a';
    ps1.pc1 = &c;
    *(unsigned int *)(wiredata + 4) = 0xdeadbeef;
    if (use_pointer_ids)
    {
	*(unsigned int *)(wiredata + 8) = 0x20000;
	*(unsigned int *)(wiredata + 12) = 0x20004;
    }
    else
    {
	*(unsigned int *)(wiredata + 8) = (unsigned int)&l;
	*(unsigned int *)(wiredata + 12) = (unsigned int)&c;
    }
    memcpy(wiredata + 16, &l, 4);
    memcpy(wiredata + 20, &c, 1);

    test_simple_struct_marshal(fmtstr_pointer_struct + 4, &ps1, 17, wiredata + 4, 17, ps1_cmp, 2, "pointer_struct");
    if (use_pointer_ids)
        *(unsigned int *)wiredata = 0x20000;
    else
        *(unsigned int *)wiredata = (unsigned int)&ps1;
    if (0)
    {
    /* one of the unmarshallings crashes Wine */
    test_pointer_marshal(fmtstr_pointer_struct, &ps1, 17, wiredata, 21, ps1_cmp, 2, "pointer_struct");
    }
}

static void test_fullpointer_xlat(void)
{
    PFULL_PTR_XLAT_TABLES pXlatTables;
    ULONG RefId;
    int ret;
    void *Pointer;

    pXlatTables = NdrFullPointerXlatInit(2, XLAT_CLIENT);

    /* "marshaling" phase */

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xcafebeef, 1, &RefId);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(RefId == 0x1, "RefId should be 0x1 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xcafebeef, 0, &RefId);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(RefId == 0x1, "RefId should be 0x1 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xcafebabe, 0, &RefId);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(RefId == 0x2, "RefId should be 0x2 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xdeadbeef, 0, &RefId);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(RefId == 0x3, "RefId should be 0x3 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, NULL, 0, &RefId);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);
    ok(RefId == 0, "RefId should be 0 instead of 0x%x\n", RefId);

    /* "unmarshaling" phase */

    ret = NdrFullPointerQueryRefId(pXlatTables, 0x2, 0, &Pointer);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(Pointer == (void *)0xcafebabe, "Pointer should be 0xcafebabe instead of %p\n", Pointer);

    ret = NdrFullPointerQueryRefId(pXlatTables, 0x4, 0, &Pointer);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(Pointer == NULL, "Pointer should be NULL instead of %p\n", Pointer);

    NdrFullPointerInsertRefId(pXlatTables, 0x4, (void *)0xdeadbabe);

    ret = NdrFullPointerQueryRefId(pXlatTables, 0x4, 1, &Pointer);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(Pointer == (void *)0xdeadbabe, "Pointer should be (void *)0xdeadbabe instead of %p\n", Pointer);

    NdrFullPointerXlatFree(pXlatTables);

    pXlatTables = NdrFullPointerXlatInit(2, XLAT_SERVER);

    /* "unmarshaling" phase */

    ret = NdrFullPointerQueryRefId(pXlatTables, 0x2, 1, &Pointer);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(Pointer == NULL, "Pointer should be NULL instead of %p\n", Pointer);

    NdrFullPointerInsertRefId(pXlatTables, 0x2, (void *)0xcafebabe);

    ret = NdrFullPointerQueryRefId(pXlatTables, 0x2, 0, &Pointer);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(Pointer == (void *)0xcafebabe, "Pointer should be (void *)0xcafebabe instead of %p\n", Pointer);

    ret = NdrFullPointerQueryRefId(pXlatTables, 0x2, 1, &Pointer);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);
    ok(Pointer == (void *)0xcafebabe, "Pointer should be (void *)0xcafebabe instead of %p\n", Pointer);

    /* "marshaling" phase */

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xcafebeef, 1, &RefId);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(RefId == 0x3, "RefId should be 0x3 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xcafebeef, 1, &RefId);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);
    ok(RefId == 0x3, "RefId should be 0x3 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xcafebeef, 0, &RefId);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(RefId == 0x3, "RefId should be 0x3 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xcafebabe, 0, &RefId);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(RefId == 0x2, "RefId should be 0x2 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xdeadbeef, 0, &RefId);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(RefId == 0x4, "RefId should be 0x4 instead of 0x%x\n", RefId);

    /* "freeing" phase */

    ret = NdrFullPointerFree(pXlatTables, (void *)0xcafebeef);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xcafebeef, 0x20, &RefId);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);
    ok(RefId == 0x3, "RefId should be 0x3 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xcafebeef, 1, &RefId);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);
    ok(RefId == 0x3, "RefId should be 0x3 instead of 0x%x\n", RefId);

    ret = NdrFullPointerFree(pXlatTables, (void *)0xcafebabe);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);

    ret = NdrFullPointerFree(pXlatTables, (void *)0xdeadbeef);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xdeadbeef, 0x20, &RefId);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);
    ok(RefId == 0x4, "RefId should be 0x4 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xdeadbeef, 1, &RefId);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(RefId == 0x4, "RefId should be 0x4 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xdeadbeef, 1, &RefId);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);
    ok(RefId == 0x4, "RefId should be 0x4 instead of 0x%x\n", RefId);

    ret = NdrFullPointerFree(pXlatTables, (void *)0xdeadbeef);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);

    NdrFullPointerXlatFree(pXlatTables);
}

static void test_client_init(void)
{
    MIDL_STUB_MESSAGE stubMsg;
    RPC_MESSAGE rpcMsg;

    memset(&rpcMsg, 0xcc, sizeof(rpcMsg));
    memset(&stubMsg, 0xcc, sizeof(stubMsg));

    NdrClientInitializeNew(&rpcMsg, &stubMsg, &Object_StubDesc, 1);

#define TEST_POINTER_UNSET(field) ok(rpcMsg.field == (void *)0xcccccccc, #field " should have been unset instead of %p\n", rpcMsg.field)

    ok(rpcMsg.Handle == NULL, "rpcMsg.Handle should have been NULL instead of %p\n", rpcMsg.Handle);
    TEST_POINTER_UNSET(Buffer);
    ok(rpcMsg.BufferLength == 0xcccccccc, "rpcMsg.BufferLength should have been unset instead of %d\n", rpcMsg.BufferLength);
    ok(rpcMsg.ProcNum == 0x8001, "rpcMsg.ProcNum should have been 0x8001 instead of 0x%x\n", rpcMsg.ProcNum);
    TEST_POINTER_UNSET(TransferSyntax);
    ok(rpcMsg.RpcInterfaceInformation == Object_StubDesc.RpcInterfaceInformation,
        "rpcMsg.RpcInterfaceInformation should have been %p instead of %p\n",
        Object_StubDesc.RpcInterfaceInformation, rpcMsg.RpcInterfaceInformation);
    /* Note: ReservedForRuntime not tested */
    TEST_POINTER_UNSET(ManagerEpv);
    TEST_POINTER_UNSET(ImportContext);
    ok(rpcMsg.RpcFlags == 0, "rpcMsg.RpcFlags should have been 0 instead of 0x%lx\n", rpcMsg.RpcFlags);
#undef TEST_POINTER_UNSET

#define TEST_ZERO(field, fmt) ok(stubMsg.field == 0, #field " should have been set to zero instead of " fmt "\n", stubMsg.field)
#define TEST_POINTER_UNSET(field) ok(stubMsg.field == (void *)0xcccccccc, #field " should have been unset instead of %p\n", stubMsg.field)
#define TEST_ULONG_UNSET(field) ok(stubMsg.field == 0xcccccccc, #field " should have been unset instead of 0x%x\n", stubMsg.field)
#define TEST_ULONG_PTR_UNSET(field) ok(stubMsg.field == 0xcccccccc, #field " should have been unset instead of 0x%lx\n", stubMsg.field)

    ok(stubMsg.RpcMsg == &rpcMsg, "stubMsg.RpcMsg should have been %p instead of %p\n", &rpcMsg, stubMsg.RpcMsg);
    TEST_POINTER_UNSET(Buffer);
    TEST_ZERO(BufferStart, "%p");
    TEST_ZERO(BufferEnd, "%p");
    TEST_POINTER_UNSET(BufferMark);
    TEST_ZERO(BufferLength, "%d");
    TEST_ULONG_UNSET(MemorySize);
    TEST_POINTER_UNSET(Memory);
    ok(stubMsg.IsClient == 1, "stubMsg.IsClient should have been 1 instead of %u\n", stubMsg.IsClient);
    TEST_ZERO(ReuseBuffer, "%d");
    TEST_ZERO(pAllocAllNodesContext, "%p");
    TEST_ZERO(pPointerQueueState, "%p");
    TEST_ZERO(IgnoreEmbeddedPointers, "%d");
    TEST_ZERO(PointerBufferMark, "%p");
    TEST_ZERO(CorrDespIncrement, "%d");
    TEST_ZERO(uFlags, "%d");
    /* FIXME: UniquePtrCount */
    TEST_ULONG_PTR_UNSET(MaxCount);
    TEST_ULONG_UNSET(Offset);
    TEST_ULONG_UNSET(ActualCount);
    ok(stubMsg.pfnAllocate == my_alloc, "stubMsg.pfnAllocate should have been %p instead of %p\n", my_alloc, stubMsg.pfnAllocate);
    ok(stubMsg.pfnFree == my_free, "stubMsg.pfnFree should have been %p instead of %p\n", my_free, stubMsg.pfnFree);
    TEST_ZERO(StackTop, "%p");
    TEST_POINTER_UNSET(pPresentedType);
    TEST_POINTER_UNSET(pTransmitType);
    TEST_POINTER_UNSET(SavedHandle);
    ok(stubMsg.StubDesc == &Object_StubDesc, "stubMsg.StubDesc should have been %p instead of %p\n", &Object_StubDesc, stubMsg.StubDesc);
    TEST_POINTER_UNSET(FullPtrXlatTables);
    TEST_ZERO(FullPtrRefId, "%d");
    TEST_ZERO(PointerLength, "%d");
    TEST_ZERO(fInDontFree, "%d");
    TEST_ZERO(fDontCallFreeInst, "%d");
    TEST_ZERO(fInOnlyParam, "%d");
    TEST_ZERO(fHasReturn, "%d");
    TEST_ZERO(fHasExtensions, "%d");
    TEST_ZERO(fHasNewCorrDesc, "%d");
    TEST_ZERO(fIsIn, "%d");
    TEST_ZERO(fIsOut, "%d");
    TEST_ZERO(fIsOicf, "%d");
    TEST_ZERO(fBufferValid, "%d");
    TEST_ZERO(fHasMemoryValidateCallback, "%d");
    TEST_ZERO(fInFree, "%d");
    TEST_ZERO(fNeedMCCP, "%d");
    TEST_ZERO(fUnused, "0x%x");
    ok(stubMsg.fUnused2 == 0xffffcccc, "stubMsg.fUnused2 should have been 0xffffcccc instead of 0x%x\n", stubMsg.fUnused2);
    ok(stubMsg.dwDestContext == MSHCTX_DIFFERENTMACHINE, "stubMsg.dwDestContext should have been MSHCTX_DIFFERENTMACHINE instead of %d\n", stubMsg.dwDestContext);
    TEST_ZERO(pvDestContext, "%p");
    TEST_POINTER_UNSET(SavedContextHandles);
    TEST_ULONG_UNSET(ParamNumber);
    TEST_ZERO(pRpcChannelBuffer, "%p");
    TEST_ZERO(pArrayInfo, "%p");
    TEST_POINTER_UNSET(SizePtrCountArray);
    TEST_POINTER_UNSET(SizePtrOffsetArray);
    TEST_POINTER_UNSET(SizePtrLengthArray);
    TEST_POINTER_UNSET(pArgQueue);
    TEST_ZERO(dwStubPhase, "%d");
    /* FIXME: where does this value come from? */
    trace("LowStackMark is %p\n", stubMsg.LowStackMark);
    TEST_ZERO(pAsyncMsg, "%p");
    TEST_ZERO(pCorrInfo, "%p");
    TEST_ZERO(pCorrMemory, "%p");
    TEST_ZERO(pMemoryList, "%p");
    TEST_POINTER_UNSET(pCSInfo);
    TEST_POINTER_UNSET(ConformanceMark);
    TEST_POINTER_UNSET(VarianceMark);
    ok(stubMsg.Unused == 0xcccccccc, "Unused should have be unset instead of 0x%lx\n", stubMsg.Unused);
    TEST_POINTER_UNSET(pContext);
    TEST_POINTER_UNSET(ContextHandleHash);
    TEST_POINTER_UNSET(pUserMarshalList);
    TEST_ULONG_PTR_UNSET(Reserved51_3);
    TEST_ULONG_PTR_UNSET(Reserved51_4);
    TEST_ULONG_PTR_UNSET(Reserved51_5);
#undef TEST_ULONG_UNSET
#undef TEST_POINTER_UNSET
#undef TEST_ZERO

}

static void test_server_init(void)
{
    MIDL_STUB_MESSAGE stubMsg;
    RPC_MESSAGE rpcMsg;
    unsigned char *ret;
    unsigned char buffer[256];

    memset(&rpcMsg, 0, sizeof(rpcMsg));
    rpcMsg.Buffer = buffer;
    rpcMsg.BufferLength = sizeof(buffer);
    rpcMsg.RpcFlags = RPC_BUFFER_COMPLETE;

    memset(&stubMsg, 0xcc, sizeof(stubMsg));

    ret = NdrServerInitializeNew(&rpcMsg, &stubMsg, &Object_StubDesc);
    ok(ret == NULL, "NdrServerInitializeNew should have returned NULL instead of %p\n", ret);

#define TEST_ZERO(field, fmt) ok(stubMsg.field == 0, #field " should have been set to zero instead of " fmt "\n", stubMsg.field)
#define TEST_POINTER_UNSET(field) ok(stubMsg.field == (void *)0xcccccccc, #field " should have been unset instead of %p\n", stubMsg.field)
#define TEST_ULONG_UNSET(field) ok(stubMsg.field == 0xcccccccc, #field " should have been unset instead of 0x%x\n", stubMsg.field)
#define TEST_ULONG_PTR_UNSET(field) ok(stubMsg.field == 0xcccccccc, #field " should have been unset instead of 0x%lx\n", stubMsg.field)

    ok(stubMsg.RpcMsg == &rpcMsg, "stubMsg.RpcMsg should have been %p instead of %p\n", &rpcMsg, stubMsg.RpcMsg);
    ok(stubMsg.Buffer == buffer, "stubMsg.Buffer should have been %p instead of %p\n", buffer, stubMsg.Buffer);
    ok(stubMsg.BufferStart == buffer, "stubMsg.BufferStart should have been %p instead of %p\n", buffer, stubMsg.BufferStart);
    ok(stubMsg.BufferEnd == buffer + sizeof(buffer), "stubMsg.BufferEnd should have been %p instead of %p\n", buffer + sizeof(buffer), stubMsg.BufferEnd);
    TEST_POINTER_UNSET(BufferMark);
todo_wine
    TEST_ZERO(BufferLength, "%d");
    TEST_ULONG_UNSET(MemorySize);
    TEST_POINTER_UNSET(Memory);
    ok(stubMsg.IsClient == 0, "stubMsg.IsClient should have been 0 instead of %u\n", stubMsg.IsClient);
    TEST_ZERO(ReuseBuffer, "%d");
    TEST_ZERO(pAllocAllNodesContext, "%p");
    TEST_ZERO(pPointerQueueState, "%p");
    TEST_ZERO(IgnoreEmbeddedPointers, "%d");
    TEST_ZERO(PointerBufferMark, "%p");
    ok(stubMsg.CorrDespIncrement == 0xcc, "CorrDespIncrement should have been unset instead of 0x%x\n", stubMsg.CorrDespIncrement);
    TEST_ZERO(uFlags, "%d");
    /* FIXME: UniquePtrCount */
    TEST_ULONG_PTR_UNSET(MaxCount);
    TEST_ULONG_UNSET(Offset);
    TEST_ULONG_UNSET(ActualCount);
    ok(stubMsg.pfnAllocate == my_alloc, "stubMsg.pfnAllocate should have been %p instead of %p\n", my_alloc, stubMsg.pfnAllocate);
    ok(stubMsg.pfnFree == my_free, "stubMsg.pfnFree should have been %p instead of %p\n", my_free, stubMsg.pfnFree);
    TEST_ZERO(StackTop, "%p");
    TEST_POINTER_UNSET(pPresentedType);
    TEST_POINTER_UNSET(pTransmitType);
    TEST_POINTER_UNSET(SavedHandle);
    ok(stubMsg.StubDesc == &Object_StubDesc, "stubMsg.StubDesc should have been %p instead of %p\n", &Object_StubDesc, stubMsg.StubDesc);
    TEST_ZERO(FullPtrXlatTables, "%p");
    TEST_ZERO(FullPtrRefId, "%d");
    TEST_ZERO(PointerLength, "%d");
    TEST_ZERO(fInDontFree, "%d");
    TEST_ZERO(fDontCallFreeInst, "%d");
    TEST_ZERO(fInOnlyParam, "%d");
    TEST_ZERO(fHasReturn, "%d");
    TEST_ZERO(fHasExtensions, "%d");
    TEST_ZERO(fHasNewCorrDesc, "%d");
    TEST_ZERO(fIsIn, "%d");
    TEST_ZERO(fIsOut, "%d");
    TEST_ZERO(fIsOicf, "%d");
    trace("fBufferValid = %d\n", stubMsg.fBufferValid);
    TEST_ZERO(fHasMemoryValidateCallback, "%d");
    TEST_ZERO(fInFree, "%d");
    TEST_ZERO(fNeedMCCP, "%d");
    TEST_ZERO(fUnused, "0x%x");
    ok(stubMsg.fUnused2 == 0xffffcccc, "stubMsg.fUnused2 should have been 0xffffcccc instead of 0x%x\n", stubMsg.fUnused2);
    ok(stubMsg.dwDestContext == MSHCTX_DIFFERENTMACHINE, "stubMsg.dwDestContext should have been MSHCTX_DIFFERENTMACHINE instead of %d\n", stubMsg.dwDestContext);
    TEST_ZERO(pvDestContext, "%p");
    TEST_POINTER_UNSET(SavedContextHandles);
    TEST_ULONG_UNSET(ParamNumber);
    TEST_ZERO(pRpcChannelBuffer, "%p");
    TEST_ZERO(pArrayInfo, "%p");
    TEST_POINTER_UNSET(SizePtrCountArray);
    TEST_POINTER_UNSET(SizePtrOffsetArray);
    TEST_POINTER_UNSET(SizePtrLengthArray);
    TEST_POINTER_UNSET(pArgQueue);
    TEST_ZERO(dwStubPhase, "%d");
    /* FIXME: where does this value come from? */
    trace("LowStackMark is %p\n", stubMsg.LowStackMark);
    TEST_ZERO(pAsyncMsg, "%p");
    TEST_ZERO(pCorrInfo, "%p");
    TEST_ZERO(pCorrMemory, "%p");
    TEST_ZERO(pMemoryList, "%p");
    TEST_POINTER_UNSET(pCSInfo);
    TEST_POINTER_UNSET(ConformanceMark);
    TEST_POINTER_UNSET(VarianceMark);
    ok(stubMsg.Unused == 0xcccccccc, "Unused should have be unset instead of 0x%lx\n", stubMsg.Unused);
    TEST_POINTER_UNSET(pContext);
    TEST_POINTER_UNSET(ContextHandleHash);
    TEST_POINTER_UNSET(pUserMarshalList);
    TEST_ULONG_PTR_UNSET(Reserved51_3);
    TEST_ULONG_PTR_UNSET(Reserved51_4);
    TEST_ULONG_PTR_UNSET(Reserved51_5);
#undef TEST_ULONG_UNSET
#undef TEST_POINTER_UNSET
#undef TEST_ZERO

}

static void test_ndr_allocate(void)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    void *p1, *p2;
    struct tag_mem_list_v1_t
    {
        DWORD magic;
        void *ptr;
        struct tag_mem_list_v1_t *next;
    } *mem_list_v1;
    struct tag_mem_list_v2_t
    {
        DWORD magic;
        DWORD size;
        DWORD unknown;
        struct tag_mem_list_v2_t *next;
    } *mem_list_v2;
    const DWORD magic_MEML = 'M' << 24 | 'E' << 16 | 'M' << 8 | 'L';

    StubDesc = Object_StubDesc;
    NdrClientInitializeNew(&RpcMessage, &StubMsg, &StubDesc, 0);

    ok(StubMsg.pMemoryList == NULL, "memlist %p\n", StubMsg.pMemoryList);
    my_alloc_called = my_free_called = 0;
    p1 = NdrAllocate(&StubMsg, 10);
    p2 = NdrAllocate(&StubMsg, 24);
    ok(my_alloc_called == 2, "alloc called %d\n", my_alloc_called);
    ok(StubMsg.pMemoryList != NULL, "StubMsg.pMemoryList NULL\n");
    if(StubMsg.pMemoryList)
    {
        mem_list_v2 = StubMsg.pMemoryList;
        if (mem_list_v2->size == 24)
        {
            trace("v2 mem list format\n");
            ok((char *)mem_list_v2 == (char *)p2 + 24, "expected mem_list_v2 pointer %p, but got %p\n", (char *)p2 + 24, mem_list_v2);
            ok(mem_list_v2->magic == magic_MEML, "magic %08x\n", mem_list_v2->magic);
            ok(mem_list_v2->size == 24, "wrong size for p2 %d\n", mem_list_v2->size);
            ok(mem_list_v2->unknown == 0, "wrong unknown for p2 0x%x\n", mem_list_v2->unknown);
            ok(mem_list_v2->next != NULL, "next NULL\n");
            mem_list_v2 = mem_list_v2->next;
            if(mem_list_v2)
            {
                ok((char *)mem_list_v2 == (char *)p1 + 16, "expected mem_list_v2 pointer %p, but got %p\n", (char *)p1 + 16, mem_list_v2);
                ok(mem_list_v2->magic == magic_MEML, "magic %08x\n", mem_list_v2->magic);
                ok(mem_list_v2->size == 16, "wrong size for p1 %d\n", mem_list_v2->size);
                ok(mem_list_v2->unknown == 0, "wrong unknown for p1 0x%x\n", mem_list_v2->unknown);
                ok(mem_list_v2->next == NULL, "next %p\n", mem_list_v2->next);
            }
        }
        else
        {
            trace("v1 mem list format\n");
            mem_list_v1 = StubMsg.pMemoryList;
            ok(mem_list_v1->magic == magic_MEML, "magic %08x\n", mem_list_v1->magic);
            ok(mem_list_v1->ptr == p2, "ptr != p2\n");
            ok(mem_list_v1->next != NULL, "next NULL\n");
            mem_list_v1 = mem_list_v1->next;
            if(mem_list_v1)
            {
                ok(mem_list_v1->magic == magic_MEML, "magic %08x\n", mem_list_v1->magic);
                ok(mem_list_v1->ptr == p1, "ptr != p1\n");
                ok(mem_list_v1->next == NULL, "next %p\n", mem_list_v1->next);
            }
        }
    }
    /* NdrFree isn't exported so we can't test free'ing */
}

static void test_conformant_array(void)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    void *ptr;
    unsigned char *mem, *mem_orig;
    unsigned char memsrc[20];
    unsigned int i;

    static const unsigned char fmtstr_conf_array[] =
    {
        0x1b,              /* FC_CARRAY */
        0x0,               /* align */
        NdrFcShort( 0x1 ), /* elem size */
        0x40,              /* Corr desc:  const */
        0x0,
        NdrFcShort(0x10),  /* const = 0x10 */
        0x1,               /* FC_BYTE */
        0x5b               /* FC_END */
    };

    for (i = 0; i < sizeof(memsrc); i++)
        memsrc[i] = i * i;

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = fmtstr_conf_array;

    NdrClientInitializeNew(
                           &RpcMessage,
                           &StubMsg,
                           &StubDesc,
                           0);

    StubMsg.BufferLength = 0;
    NdrConformantArrayBufferSize( &StubMsg,
                          memsrc,
                          fmtstr_conf_array );
    ok(StubMsg.BufferLength >= 20, "length %d\n", StubMsg.BufferLength);

    /*NdrGetBuffer(&_StubMsg, _StubMsg.BufferLength, NULL);*/
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    StubMsg.BufferEnd = StubMsg.BufferStart + StubMsg.BufferLength;

    ptr = NdrConformantArrayMarshall( &StubMsg,  memsrc, fmtstr_conf_array );
    ok(ptr == NULL, "ret %p\n", ptr);
    ok(StubMsg.Buffer - StubMsg.BufferStart == 20, "Buffer %p Start %p len %d\n", StubMsg.Buffer, StubMsg.BufferStart, 20);
    ok(!memcmp(StubMsg.BufferStart + 4, memsrc, 16), "incorrectly marshaled\n");

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;
    mem = NULL;

    /* Client */
    my_alloc_called = 0;
    /* passing mem == NULL with must_alloc == 0 crashes under Windows */
    NdrConformantArrayUnmarshall( &StubMsg, &mem, fmtstr_conf_array, 1);
    ok(mem != NULL, "mem not alloced\n");
    ok(mem != StubMsg.BufferStart + 4, "mem pointing at buffer\n");
    ok(my_alloc_called == 1, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    mem_orig = mem;
    NdrConformantArrayUnmarshall( &StubMsg, &mem, fmtstr_conf_array, 0);
    ok(mem == mem_orig, "mem alloced\n");
    ok(mem != StubMsg.BufferStart + 4, "mem pointing at buffer\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrConformantArrayUnmarshall( &StubMsg, &mem, fmtstr_conf_array, 1);
    ok(mem != mem_orig, "mem not alloced\n");
    ok(mem != StubMsg.BufferStart + 4, "mem pointing at buffer\n");
    ok(my_alloc_called == 1, "alloc called %d\n", my_alloc_called);

    my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrConformantArrayFree( &StubMsg, mem, fmtstr_conf_array );
    ok(my_free_called == 0, "free called %d\n", my_free_called);
    StubMsg.pfnFree(mem);

    /* Server */
    my_alloc_called = 0;
    StubMsg.IsClient = 0;
    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrConformantArrayUnmarshall( &StubMsg, &mem, fmtstr_conf_array, 0);
    ok(mem == StubMsg.BufferStart + 4, "mem not pointing at buffer\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);
    my_alloc_called = 0;
    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrConformantArrayUnmarshall( &StubMsg, &mem, fmtstr_conf_array, 1);
    ok(mem != StubMsg.BufferStart + 4, "mem pointing at buffer\n");
    ok(my_alloc_called == 1, "alloc called %d\n", my_alloc_called);
    StubMsg.pfnFree(mem);

    my_alloc_called = 0;
    mem = mem_orig;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrConformantArrayUnmarshall( &StubMsg, &mem, fmtstr_conf_array, 0);
    ok(mem == mem_orig, "mem alloced\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    mem = mem_orig;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrConformantArrayUnmarshall( &StubMsg, &mem, fmtstr_conf_array, 1);
    ok(mem != StubMsg.BufferStart + 4, "mem pointing at buffer\n");
    ok(my_alloc_called == 1, "alloc called %d\n", my_alloc_called);
    StubMsg.pfnFree(mem);
    StubMsg.pfnFree(mem_orig);

    HeapFree(GetProcessHeap(), 0, StubMsg.RpcMsg->Buffer);
}

static void test_conformant_string(void)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    void *ptr;
    unsigned char *mem, *mem_orig;
    char memsrc[] = "This is a test string";

    static const unsigned char fmtstr_conf_str[] =
    {
			0x11, 0x8,	/* FC_RP [simple_pointer] */
			0x22,		/* FC_C_CSTRING */
			0x5c,		/* FC_PAD */
    };

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = fmtstr_conf_str;

    NdrClientInitializeNew(
                           &RpcMessage,
                           &StubMsg,
                           &StubDesc,
                           0);

    StubMsg.BufferLength = 0;
    NdrPointerBufferSize( &StubMsg,
                          (unsigned char *)memsrc,
                          fmtstr_conf_str );
    ok(StubMsg.BufferLength >= sizeof(memsrc) + 12, "length %d\n", StubMsg.BufferLength);

    /*NdrGetBuffer(&_StubMsg, _StubMsg.BufferLength, NULL);*/
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    StubMsg.BufferEnd = StubMsg.BufferStart + StubMsg.BufferLength;

    ptr = NdrPointerMarshall( &StubMsg, (unsigned char *)memsrc, fmtstr_conf_str );
    ok(ptr == NULL, "ret %p\n", ptr);
    ok(StubMsg.Buffer - StubMsg.BufferStart == sizeof(memsrc) + 12, "Buffer %p Start %p len %d\n",
       StubMsg.Buffer, StubMsg.BufferStart, StubMsg.Buffer - StubMsg.BufferStart);
    ok(!memcmp(StubMsg.BufferStart + 12, memsrc, sizeof(memsrc)), "incorrectly marshaled\n");

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;
    mem = NULL;

    /* Client */
    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    mem = mem_orig = HeapAlloc(GetProcessHeap(), 0, sizeof(memsrc));
    NdrPointerUnmarshall( &StubMsg, &mem, fmtstr_conf_str, 0);
    ok(mem == mem_orig, "mem not alloced\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, fmtstr_conf_str, 1);
todo_wine {
    ok(mem == mem_orig, "mem not alloced\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);
}

    my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerFree( &StubMsg, mem, fmtstr_conf_str );
    ok(my_free_called == 1, "free called %d\n", my_free_called);

    mem = my_alloc(10);
    my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerFree( &StubMsg, mem, fmtstr_conf_str );
    ok(my_free_called == 1, "free called %d\n", my_free_called);

    /* Server */
    my_alloc_called = 0;
    StubMsg.IsClient = 0;
    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, fmtstr_conf_str, 0);
    ok(mem == StubMsg.BufferStart + 12, "mem not pointing at buffer\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, fmtstr_conf_str, 1);
todo_wine {
    ok(mem == StubMsg.BufferStart + 12, "mem not pointing at buffer\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);
}

    my_alloc_called = 0;
    mem = mem_orig;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, fmtstr_conf_str, 0);
    ok(mem == StubMsg.BufferStart + 12, "mem not pointing at buffer\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    mem = mem_orig;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, fmtstr_conf_str, 1);
todo_wine {
    ok(mem == StubMsg.BufferStart + 12, "mem not pointing at buffer\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);
}

    mem = my_alloc(10);
    my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerFree( &StubMsg, mem, fmtstr_conf_str );
    ok(my_free_called == 1, "free called %d\n", my_free_called);

    HeapFree(GetProcessHeap(), 0, mem_orig);
    HeapFree(GetProcessHeap(), 0, StubMsg.RpcMsg->Buffer);
}

static void test_nonconformant_string(void)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    void *ptr;
    unsigned char *mem, *mem_orig;
    unsigned char memsrc[10] = "This is";
    unsigned char memsrc2[10] = "This is a";

    static const unsigned char fmtstr_nonconf_str[] =
    {
			0x26,		/* FC_CSTRING */
			0x5c,		/* FC_PAD */
			NdrFcShort( 0xa ),	/* 10 */
    };

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = fmtstr_nonconf_str;

    /* length < size */
    NdrClientInitializeNew(
                           &RpcMessage,
                           &StubMsg,
                           &StubDesc,
                           0);

    StubMsg.BufferLength = 0;

    NdrNonConformantStringBufferSize( &StubMsg,
                          (unsigned char *)memsrc,
                          fmtstr_nonconf_str );
    ok(StubMsg.BufferLength >= strlen((char *)memsrc) + 1 + 8, "length %d\n", StubMsg.BufferLength);

    /*NdrGetBuffer(&_StubMsg, _StubMsg.BufferLength, NULL);*/
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    StubMsg.BufferEnd = StubMsg.BufferStart + StubMsg.BufferLength;

    ptr = NdrNonConformantStringMarshall( &StubMsg, (unsigned char *)memsrc, fmtstr_nonconf_str );
    ok(ptr == NULL, "ret %p\n", ptr);
    ok(StubMsg.Buffer - StubMsg.BufferStart == strlen((char *)memsrc) + 1 + 8, "Buffer %p Start %p len %d\n",
       StubMsg.Buffer, StubMsg.BufferStart, StubMsg.Buffer - StubMsg.BufferStart);
    ok(!memcmp(StubMsg.BufferStart + 8, memsrc, strlen((char *)memsrc) + 1), "incorrectly marshaled\n");

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;
    mem = NULL;

    /* Client */
    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    mem = mem_orig = HeapAlloc(GetProcessHeap(), 0, sizeof(memsrc));
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 0);
    ok(mem == mem_orig, "mem alloced\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 1);
    todo_wine
    ok(mem == mem_orig, "mem alloced\n");
    todo_wine
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    /* Server */
    my_alloc_called = 0;
    StubMsg.IsClient = 0;
    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 0);
    ok(mem != mem_orig, "mem not alloced\n");
    ok(mem != StubMsg.BufferStart + 8, "mem pointing at buffer\n");
    ok(my_alloc_called == 1, "alloc called %d\n", my_alloc_called);
    NdrOleFree(mem);

    my_alloc_called = 0;
    mem = mem_orig;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 0);
    ok(mem == mem_orig, "mem alloced\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    mem = mem_orig;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 1);
    todo_wine
    ok(mem == mem_orig, "mem alloced\n");
    todo_wine
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    HeapFree(GetProcessHeap(), 0, mem_orig);
    HeapFree(GetProcessHeap(), 0, StubMsg.RpcMsg->Buffer);

    /* length = size */
    NdrClientInitializeNew(
                           &RpcMessage,
                           &StubMsg,
                           &StubDesc,
                           0);

    StubMsg.BufferLength = 0;

    NdrNonConformantStringBufferSize( &StubMsg,
                          (unsigned char *)memsrc2,
                          fmtstr_nonconf_str );
    ok(StubMsg.BufferLength >= strlen((char *)memsrc2) + 1 + 8, "length %d\n", StubMsg.BufferLength);

    /*NdrGetBuffer(&_StubMsg, _StubMsg.BufferLength, NULL);*/
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    StubMsg.BufferEnd = StubMsg.BufferStart + StubMsg.BufferLength;

    ptr = NdrNonConformantStringMarshall( &StubMsg, (unsigned char *)memsrc2, fmtstr_nonconf_str );
    ok(ptr == NULL, "ret %p\n", ptr);
    ok(StubMsg.Buffer - StubMsg.BufferStart == strlen((char *)memsrc2) + 1 + 8, "Buffer %p Start %p len %d\n",
       StubMsg.Buffer, StubMsg.BufferStart, StubMsg.Buffer - StubMsg.BufferStart);
    ok(!memcmp(StubMsg.BufferStart + 8, memsrc2, strlen((char *)memsrc2) + 1), "incorrectly marshaled\n");

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;
    mem = NULL;

    /* Client */
    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    mem = mem_orig = HeapAlloc(GetProcessHeap(), 0, sizeof(memsrc));
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 0);
    ok(mem == mem_orig, "mem alloced\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 1);
    todo_wine
    ok(mem == mem_orig, "mem alloced\n");
    todo_wine
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    /* Server */
    my_alloc_called = 0;
    StubMsg.IsClient = 0;
    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 0);
    ok(mem != mem_orig, "mem not alloced\n");
    ok(mem != StubMsg.BufferStart + 8, "mem pointing at buffer\n");
    ok(my_alloc_called == 1, "alloc called %d\n", my_alloc_called);
    NdrOleFree(mem);

    my_alloc_called = 0;
    mem = mem_orig;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 0);
    ok(mem == mem_orig, "mem alloced\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    mem = mem_orig;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 1);
    todo_wine
    ok(mem == mem_orig, "mem alloced\n");
    todo_wine
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    HeapFree(GetProcessHeap(), 0, mem_orig);
    HeapFree(GetProcessHeap(), 0, StubMsg.RpcMsg->Buffer);
}

static void test_ndr_buffer(void)
{
    static unsigned char ncalrpc[] = "ncalrpc";
    static unsigned char endpoint[] = "winetest:test_ndr_buffer";
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc = Object_StubDesc;
    unsigned char *ret;
    unsigned char *binding;
    RPC_BINDING_HANDLE Handle;
    RPC_STATUS status;
    ULONG prev_buffer_length;
    BOOL old_buffer_valid_location;

    StubDesc.RpcInterfaceInformation = (void *)&IFoo___RpcServerInterface;

    status = RpcServerUseProtseqEp(ncalrpc, 20, endpoint, NULL);
    ok(RPC_S_OK == status, "RpcServerUseProtseqEp failed with status %lu\n", status);
    status = RpcServerRegisterIf(IFoo_v0_0_s_ifspec, NULL, NULL);
    ok(RPC_S_OK == status, "RpcServerRegisterIf failed with status %lu\n", status);
    status = RpcServerListen(1, 20, TRUE);
    ok(RPC_S_OK == status, "RpcServerListen failed with status %lu\n", status);
    if (status != RPC_S_OK)
    {
        /* Failed to create a server, running client tests is useless */
        return;
    }

    status = RpcStringBindingCompose(NULL, ncalrpc, NULL, endpoint, NULL, &binding);
    ok(status == RPC_S_OK, "RpcStringBindingCompose failed (%lu)\n", status);

    status = RpcBindingFromStringBinding(binding, &Handle);
    ok(status == RPC_S_OK, "RpcBindingFromStringBinding failed (%lu)\n", status);
    RpcStringFree(&binding);

    NdrClientInitializeNew(&RpcMessage, &StubMsg, &StubDesc, 5);

    ret = NdrGetBuffer(&StubMsg, 10, Handle);
    ok(ret == StubMsg.Buffer, "NdrGetBuffer should have returned the same value as StubMsg.Buffer instead of %p\n", ret);
    ok(RpcMessage.Handle != NULL, "RpcMessage.Handle should not have been NULL\n");
    ok(RpcMessage.Buffer != NULL, "RpcMessage.Buffer should not have been NULL\n");
    ok(RpcMessage.BufferLength == 10, "RpcMessage.BufferLength should have been 10 instead of %d\n", RpcMessage.BufferLength);
    ok(RpcMessage.RpcFlags == 0, "RpcMessage.RpcFlags should have been 0x0 instead of 0x%lx\n", RpcMessage.RpcFlags);
    ok(StubMsg.Buffer != NULL, "Buffer should not have been NULL\n");
    ok(!StubMsg.BufferStart, "BufferStart should have been NULL instead of %p\n", StubMsg.BufferStart);
    ok(!StubMsg.BufferEnd, "BufferEnd should have been NULL instead of %p\n", StubMsg.BufferEnd);
todo_wine
    ok(StubMsg.BufferLength == 0, "BufferLength should have left as 0 instead of being set to %d\n", StubMsg.BufferLength);
    old_buffer_valid_location = !StubMsg.fBufferValid;
    if (old_buffer_valid_location)
        ok(broken(StubMsg.CorrDespIncrement == TRUE), "fBufferValid should have been TRUE instead of 0x%x\n", StubMsg.CorrDespIncrement);
    else
        ok(StubMsg.fBufferValid, "fBufferValid should have been non-zero instead of 0x%x\n", StubMsg.fBufferValid);

    prev_buffer_length = RpcMessage.BufferLength;
    StubMsg.BufferLength = 1;
    NdrFreeBuffer(&StubMsg);
    ok(RpcMessage.Handle != NULL, "RpcMessage.Handle should not have been NULL\n");
    ok(RpcMessage.Buffer != NULL, "RpcMessage.Buffer should not have been NULL\n");
    ok(RpcMessage.BufferLength == prev_buffer_length, "RpcMessage.BufferLength should have been left as %d instead of %d\n", prev_buffer_length, RpcMessage.BufferLength);
    ok(StubMsg.Buffer != NULL, "Buffer should not have been NULL\n");
    ok(StubMsg.BufferLength == 1, "BufferLength should have left as 1 instead of being set to %d\n", StubMsg.BufferLength);
    if (old_buffer_valid_location)
        ok(broken(StubMsg.CorrDespIncrement == FALSE), "fBufferValid should have been FALSE instead of 0x%x\n", StubMsg.CorrDespIncrement);
    else
        ok(!StubMsg.fBufferValid, "fBufferValid should have been FALSE instead of %d\n", StubMsg.fBufferValid);

    /* attempt double-free */
    NdrFreeBuffer(&StubMsg);

    RpcBindingFree(&Handle);

    status = RpcServerUnregisterIf(NULL, NULL, FALSE);
    ok(status == RPC_S_OK, "RpcServerUnregisterIf failed (%lu)\n", status);
}

static void test_NdrMapCommAndFaultStatus(void)
{
    RPC_STATUS rpc_status;
    MIDL_STUB_MESSAGE StubMsg;
    RPC_MESSAGE RpcMessage;

    NdrClientInitializeNew(&RpcMessage, &StubMsg, &Object_StubDesc, 5);

    for (rpc_status = 0; rpc_status < 10000; rpc_status++)
    {
        RPC_STATUS status;
        ULONG comm_status = 0;
        ULONG fault_status = 0;
        ULONG expected_comm_status = 0;
        ULONG expected_fault_status = 0;
        status = NdrMapCommAndFaultStatus(&StubMsg, &comm_status, &fault_status, rpc_status);
        ok(status == RPC_S_OK, "NdrMapCommAndFaultStatus failed with error %ld\n", status);
        switch (rpc_status)
        {
        case ERROR_INVALID_HANDLE:
        case RPC_S_INVALID_BINDING:
        case RPC_S_UNKNOWN_IF:
        case RPC_S_SERVER_UNAVAILABLE:
        case RPC_S_SERVER_TOO_BUSY:
        case RPC_S_CALL_FAILED_DNE:
        case RPC_S_PROTOCOL_ERROR:
        case RPC_S_UNSUPPORTED_TRANS_SYN:
        case RPC_S_UNSUPPORTED_TYPE:
        case RPC_S_PROCNUM_OUT_OF_RANGE:
        case EPT_S_NOT_REGISTERED:
        case RPC_S_COMM_FAILURE:
            expected_comm_status = rpc_status;
            break;
        default:
            expected_fault_status = rpc_status;
        }
        ok(comm_status == expected_comm_status, "NdrMapCommAndFaultStatus should have mapped %ld to comm status %d instead of %d\n",
            rpc_status, expected_comm_status, comm_status);
        ok(fault_status == expected_fault_status, "NdrMapCommAndFaultStatus should have mapped %ld to fault status %d instead of %d\n",
            rpc_status, expected_fault_status, fault_status);
    }
}

START_TEST( ndr_marshall )
{
    determine_pointer_marshalling_style();

    test_ndr_simple_type();
    test_simple_types();
    test_nontrivial_pointer_types();
    test_simple_struct();
    test_fullpointer_xlat();
    test_client_init();
    test_server_init();
    test_ndr_allocate();
    test_conformant_array();
    test_conformant_string();
    test_nonconformant_string();
    test_ndr_buffer();
    test_NdrMapCommAndFaultStatus();
}

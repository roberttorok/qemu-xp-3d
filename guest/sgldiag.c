/* Checks, step by step, the path from a client to the server's shared memory. */
#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <stdio.h>
#include <cfgmgr32.h>
DEFINE_GUID(G, 0xdf576976, 0x569d, 0x4672, 0x95, 0xa0, 0xf5, 0x7e, 0x4e, 0xa0, 0xb2, 0x10);
#define IOSIZE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOMAP CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
struct m { WORD peer; DWORD64 size; PVOID ptr; WORD vec; };
static void devstatus(void)
{
    HDEVINFO all = SetupDiGetClassDevsA(NULL, "PCI", NULL, DIGCF_ALLCLASSES);
    SP_DEVINFO_DATA dev = { sizeof(dev) };
    DWORD n;
    for (n = 0; SetupDiEnumDeviceInfo(all, n, &dev); n++) {
        char id[256];
        ULONG st = 0, pr = 0;
        CONFIGRET cr;
        CM_Get_Device_IDA(dev.DevInst, id, sizeof(id), 0);
        if (!strstr(id, "VEN_1AF4&DEV_1110"))
            continue;
        cr = CM_Get_DevNode_Status(&st, &pr, dev.DevInst, 0);
        printf("%s\n  cr=%lu status=%08lx problem=%lu\n", id, cr, st, pr);
    }
}

int main(void)
{
    SP_DEVICE_INTERFACE_DATA d = { sizeof(d) };
    PSP_DEVICE_INTERFACE_DETAIL_DATA_A det;
    DWORD need = 0, got = 0;
    DWORD64 size = 0;
    struct m map;
    BYTE mode = 1;
    HANDLE h;
    HDEVINFO i;
    devstatus();
    i = SetupDiGetClassDevsA(&G, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    printf("classdevs %p err %lu\n", i, GetLastError());
    if (!SetupDiEnumDeviceInterfaces(i, NULL, &G, 0, &d)) { printf("enum FAILED err %lu (driver not started?)\n", GetLastError()); return 1; }
    SetupDiGetDeviceInterfaceDetailA(i, &d, NULL, 0, &need, NULL);
    det = malloc(need); det->cbSize = sizeof(*det);
    if (!SetupDiGetDeviceInterfaceDetailA(i, &d, det, need, NULL, NULL)) { printf("detail FAILED %lu\n", GetLastError()); return 1; }
    printf("path %s\n", det->DevicePath);
    h = CreateFileA(det->DevicePath, 0, 0, NULL, OPEN_EXISTING, 0, 0);
    if (h == INVALID_HANDLE_VALUE) { printf("open FAILED %lu\n", GetLastError()); return 1; }
    if (!DeviceIoControl(h, IOSIZE, NULL, 0, &size, 8, &got, NULL)) { printf("size FAILED %lu\n", GetLastError()); return 1; }
    printf("size %lu MiB\n", (unsigned long)(size >> 20));
    if (!DeviceIoControl(h, IOMAP, &mode, 1, &map, sizeof(map), &got, NULL)) { printf("map FAILED %lu sizeof=%u\n", GetLastError(), (unsigned)sizeof(map)); return 1; }
    printf("mapped at %p\n", map.ptr);
    printf("registers: memsize %lu fifo %d gl %d.%d max_clients %d\n",
        *(DWORD*)((char*)map.ptr + 32), *(int*)((char*)map.ptr + 64), *(int*)((char*)map.ptr + 44), *(int*)((char*)map.ptr + 48), *(int*)((char*)map.ptr + 68));
    return 0;
}

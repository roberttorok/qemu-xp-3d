/* Separates the guest costs that sglbench lumps together.
   sglcost bench: shared memory write pass by pass, thread wakes with and without idle-priority spinners. Run with no 3D program open.
   sglcost hold:  keeps both CPUs out of HLT with idle-priority spinners until killed.
   sglcost clock: writes the guest's clocks to COM1 every 5 s, for comparing with the host's. */
#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <stdio.h>
#include <string.h>
#include <intrin.h>
DEFINE_GUID(G, 0xdf576976, 0x569d, 0x4672, 0x95, 0xa0, 0xf5, 0x7e, 0x4e, 0xa0, 0xb2, 0x10);
#define IOMAP CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
struct m { WORD peer; DWORD64 size; PVOID ptr; WORD vec; };

static unsigned __int64 tsc_hz;
static double secs(unsigned __int64 a, unsigned __int64 b) { return (double)(b - a) / (double)tsc_hz; }

static void calibrate(void)
{
    LARGE_INTEGER f, a, b;
    unsigned __int64 t0, t1;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&a);
    t0 = __rdtsc();
    Sleep(500);
    QueryPerformanceCounter(&b);
    t1 = __rdtsc();
    tsc_hz = (unsigned __int64)((double)(t1 - t0) * f.QuadPart / (b.QuadPart - a.QuadPart));
}

static DWORD WINAPI spinner(LPVOID unused)
{
    for (;;) {
    }
    return 0;
}

static void start_spinners(void)
{
    int cpu;
    for (cpu = 0; cpu < 2; cpu++) {
        HANDLE t = CreateThread(NULL, 0, spinner, NULL, CREATE_SUSPENDED, NULL);
        SetThreadAffinityMask(t, 1 << cpu);
        SetThreadPriority(t, THREAD_PRIORITY_IDLE);
        ResumeThread(t);
    }
}

static HANDLE ev_a, ev_b;
static DWORD WINAPI echo(LPVOID unused)
{
    for (;;) {
        WaitForSingleObject(ev_a, INFINITE);
        SetEvent(ev_b);
    }
    return 0;
}

static void wake_test(const char *label, int other_cpu)
{
    HANDLE t;
    double total = 0, worst = 0;
    int i, n = 5000;
    SetThreadAffinityMask(GetCurrentThread(), 1);
    ev_a = CreateEventA(NULL, FALSE, FALSE, NULL);
    ev_b = CreateEventA(NULL, FALSE, FALSE, NULL);
    t = CreateThread(NULL, 0, echo, NULL, CREATE_SUSPENDED, NULL);
    SetThreadAffinityMask(t, other_cpu ? 2 : 1);
    ResumeThread(t);
    for (i = 0; i < n; i++) {
        unsigned __int64 a = __rdtsc();
        double us;
        SetEvent(ev_a);
        WaitForSingleObject(ev_b, INFINITE);
        us = secs(a, __rdtsc()) * 1e6;
        total += us;
        if (us > worst)
            worst = us;
    }
    printf("%s, %s CPU: average %7.1f us, worst %8.1f us\n", label, other_cpu ? "other" : "same", total / n, worst);
    TerminateThread(t, 0);
}

static void shm_test(void)
{
    SP_DEVICE_INTERFACE_DATA d = { sizeof(d) };
    PSP_DEVICE_INTERFACE_DETAIL_DATA_A det;
    DWORD need = 0, got = 0;
    struct m map;
    BYTE mode = 1;
    HANDLE h;
    HDEVINFO i;
    size_t size = 4 * 1048576;
    char *ram, *shm;
    int pass;

    ram = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    memset(ram, 1, size);
    i = SetupDiGetClassDevsA(&G, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (!SetupDiEnumDeviceInterfaces(i, NULL, &G, 0, &d)) { printf("no driver\n"); return; }
    SetupDiGetDeviceInterfaceDetailA(i, &d, NULL, 0, &need, NULL);
    det = malloc(need); det->cbSize = sizeof(*det);
    SetupDiGetDeviceInterfaceDetailA(i, &d, det, need, NULL, NULL);
    h = CreateFileA(det->DevicePath, 0, 0, NULL, OPEN_EXISTING, 0, 0);
    if (h == INVALID_HANDLE_VALUE) { printf("open failed\n"); return; }
    if (!DeviceIoControl(h, IOMAP, &mode, 1, &map, sizeof(map), &got, NULL)) { printf("map failed\n"); return; }
    shm = (char*)map.ptr + 40 * 1048576;
    printf("shm write MB/s by pass:");
    for (pass = 0; pass < 8; pass++) {
        unsigned __int64 a = __rdtsc();
        memcpy(shm, ram, size);
        printf(" %.0f", size / 1048576.0 / secs(a, __rdtsc()));
    }
    printf("\n");
}

static void clock_log(void)
{
    HANDLE com = CreateFileA("\\\\.\\COM1", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    LARGE_INTEGER f;
    if (com == INVALID_HANDLE_VALUE) { printf("no COM1: %lu\n", GetLastError()); return; }
    QueryPerformanceFrequency(&f);
    printf("logging clocks to COM1\n");
    for (;;) {
        FILETIME ft;
        LARGE_INTEGER q;
        char line[128];
        DWORD n;
        GetSystemTimeAsFileTime(&ft);
        QueryPerformanceCounter(&q);
        n = sprintf(line, "%.3f %.3f %.3f\r\n",
            (((unsigned __int64)ft.dwHighDateTime << 32 | ft.dwLowDateTime) % 864000000000ULL) / 1e7,
            GetTickCount() / 1000.0, (double)q.QuadPart / (double)f.QuadPart);
        WriteFile(com, line, n, &n, NULL);
        Sleep(5000);
    }
}

int main(int argc, char **argv)
{
    const char *m = argc > 1 ? argv[1] : "";
    setvbuf(stdout, NULL, _IONBF, 0);
    if (!strcmp(m, "hold")) {
        start_spinners();
        Sleep(INFINITE);
    } else if (!strcmp(m, "clock")) {
        clock_log();
    } else if (!strcmp(m, "bench")) {
        calibrate();
        shm_test();
        wake_test("wake, CPUs may halt", 0);
        wake_test("wake, CPUs may halt", 1);
        start_spinners();
        Sleep(100);
        wake_test("wake, idle spinners", 0);
        wake_test("wake, idle spinners", 1);
        ExitProcess(0);
    } else {
        printf("usage: sglcost bench|hold|clock\n");
    }
    return 0;
}

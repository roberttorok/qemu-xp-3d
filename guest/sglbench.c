/* Measures what the VM costs a program: timer-tick overhead per CPU, and shared memory speed against RAM.
   Run with no 3D program open: it scribbles on an unused part of the shared memory. */
#include <windows.h>
#include <mmsystem.h>
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
    printf("tsc %.0f MHz\n", tsc_hz / 1e6);
}

/* Work done per second of real time, on one CPU. Ticks steal from this. */
static double spin_rate(int cpu)
{
    volatile unsigned long long n = 0;
    unsigned __int64 start, now;
    SetThreadAffinityMask(GetCurrentThread(), 1 << cpu);
    Sleep(50);
    start = __rdtsc();
    do {
        int i;
        for (i = 0; i < 1000; i++)
            n++;
        now = __rdtsc();
    } while (secs(start, now) < 2.0);
    return (double)n / secs(start, now);
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

static void sleep_test(DWORD ms)
{
    double total = 0, worst = 0;
    int i, n = ms < 5 ? 200 : 40;
    for (i = 0; i < n; i++) {
        unsigned __int64 a = __rdtsc();
        double t;
        Sleep(ms);
        t = secs(a, __rdtsc()) * 1000;
        total += t;
        if (t > worst)
            worst = t;
    }
    printf("Sleep(%2lu): average %6.2f ms, worst %6.2f ms\n", ms, total / n, worst);
}

static void wake_test(int other_cpu)
{
    HANDLE t;
    double total = 0, worst = 0;
    int i, n = 2000;
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
    printf("thread wake round trip, %s CPU: average %7.1f us, worst %8.1f us\n", other_cpu ? "other" : "same", total / n, worst);
    TerminateThread(t, 0);
}

static double copy_rate(void *dst, const void *src, size_t size, int rounds)
{
    unsigned __int64 a, b;
    int i;
    a = __rdtsc();
    for (i = 0; i < rounds; i++)
        memcpy(dst, src, size);
    b = __rdtsc();
    return size * (double)rounds / 1048576.0 / secs(a, b);
}

int main(void)
{
    SP_DEVICE_INTERFACE_DATA d = { sizeof(d) };
    PSP_DEVICE_INTERFACE_DETAIL_DATA_A det;
    DWORD need = 0, got = 0;
    struct m map;
    BYTE mode = 1;
    HANDLE h;
    HDEVINFO i;
    SYSTEM_INFO si;
    double a0, a1, b0, b1;
    size_t size = 4 * 1048576;
    char *ram, *ram2, *shm;

    setvbuf(stdout, NULL, _IONBF, 0);
    GetSystemInfo(&si);
    printf("cpus %lu\n", si.dwNumberOfProcessors);
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    calibrate();

    a0 = spin_rate(0);
    b0 = si.dwNumberOfProcessors > 1 ? spin_rate(1) : 0;
    timeBeginPeriod(1);
    Sleep(100);
    a1 = spin_rate(0);
    b1 = si.dwNumberOfProcessors > 1 ? spin_rate(1) : 0;
    timeEndPeriod(1);
    printf("cpu0 work at 1024Hz: %.1f%% of the rate at default ticks\n", 100 * a1 / a0);
    if (b0 > 0)
        printf("cpu1 work at 1024Hz: %.1f%% of the rate at default ticks\n", 100 * b1 / b0);

    timeBeginPeriod(1);
    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    SetThreadAffinityMask(GetCurrentThread(), 3);
    sleep_test(1);
    sleep_test(5);
    sleep_test(14);
    wake_test(0);
    if (si.dwNumberOfProcessors > 1)
        wake_test(1);
    SetThreadAffinityMask(GetCurrentThread(), 3);
    timeEndPeriod(1);

    {
        HANDLE e = CreateEventA(NULL, TRUE, FALSE, NULL);
        unsigned __int64 t0;
        int k;
        t0 = __rdtsc();
        for (k = 0; k < 100000; k++)
            SetEvent(e);
        printf("SetEvent syscall: %.2f us each\n", secs(t0, __rdtsc()) * 1e6 / 100000);
        t0 = __rdtsc();
        for (k = 0; k < 100000; k++)
            GetCurrentProcessId();
        printf("user-mode call:   %.3f us each\n", secs(t0, __rdtsc()) * 1e6 / 100000);
    }

    ram = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    ram2 = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    memset(ram, 1, size);
    memset(ram2, 2, size);
    printf("RAM copy   %8.0f MB/s\n", copy_rate(ram2, ram, size, 50));

    i = SetupDiGetClassDevsA(&G, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (!SetupDiEnumDeviceInterfaces(i, NULL, &G, 0, &d)) { printf("no driver\n"); return 1; }
    SetupDiGetDeviceInterfaceDetailA(i, &d, NULL, 0, &need, NULL);
    det = malloc(need); det->cbSize = sizeof(*det);
    SetupDiGetDeviceInterfaceDetailA(i, &d, det, need, NULL, NULL);
    h = CreateFileA(det->DevicePath, 0, 0, NULL, OPEN_EXISTING, 0, 0);
    if (h == INVALID_HANDLE_VALUE) { printf("open failed\n"); return 1; }
    if (!DeviceIoControl(h, IOMAP, &mode, 1, &map, sizeof(map), &got, NULL)) { printf("map failed\n"); return 1; }
    shm = (char*)map.ptr + 32 * 1048576;
    printf("shm write  %8.0f MB/s\n", copy_rate(shm, ram, size, 10));
    printf("shm read   %8.0f MB/s\n", copy_rate(ram2, shm, size, 10));
    return 0;
}

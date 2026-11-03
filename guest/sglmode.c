/* Sets a display mode for a few seconds, then restores it. Usage: sglmode W H BPP [SECONDS] */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv)
{
    DEVMODEA m;
    LONG r;
    if (argc < 4) return 2;
    memset(&m, 0, sizeof(m));
    m.dmSize = sizeof(m);
    m.dmPelsWidth = atoi(argv[1]);
    m.dmPelsHeight = atoi(argv[2]);
    m.dmBitsPerPel = atoi(argv[3]);
    m.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
    r = ChangeDisplaySettingsA(&m, CDS_FULLSCREEN);
    printf("result %ld\n", r);
    Sleep((argc > 4 ? atoi(argv[4]) : 6) * 1000);
    ChangeDisplaySettingsA(NULL, 0);
    return 0;
}

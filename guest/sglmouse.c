/* XP/QXL cursor diagnostic and reversible cursor-trails experiment.
 * Trails alone did not fix QXL/SDL click alignment. The working desktop
 * test disables cursor/bitmap acceleration in XP Display Properties.
 * "click" provides guest-coordinate automation while host input is broken.
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    UINT trails = 0;
    POINT pt;
    if (argc == 2 && (!strcmp(argv[1], "linear") || !strcmp(argv[1], "restore-linear"))) {
        int values[4] = {0};
        HKEY key;
        DWORD bytes = sizeof(values), type = 0;
        if (!SystemParametersInfoA(SPI_GETMOUSE, 0, values, 0) ||
            !SystemParametersInfoA(SPI_GETMOUSESPEED, 0, values + 3, 0)) return 7;
        if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\SharedGL\\MouseFix", 0,
                NULL, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &key, NULL) != ERROR_SUCCESS) return 8;
        int saved[4];
        LONG found = RegQueryValueExA(key, "OriginalPointerSettings", NULL, &type,
                                     (BYTE *)saved, &bytes);
        if (!strcmp(argv[1], "linear")) {
            if (found == ERROR_FILE_NOT_FOUND) {
                if (RegSetValueExA(key, "OriginalPointerSettings", 0, REG_BINARY,
                        (BYTE *)values, sizeof(values)) != ERROR_SUCCESS) {
                    RegCloseKey(key); return 9;
                }
            } else if (found != ERROR_SUCCESS || type != REG_BINARY || bytes != sizeof(values)) {
                RegCloseKey(key); return 10;
            }
            values[0] = values[1] = values[2] = 0;
            values[3] = 10;
        } else {
            if (found != ERROR_SUCCESS || type != REG_BINARY || bytes != sizeof(values)) {
                RegCloseKey(key); return 10;
            }
            memcpy(values, saved, sizeof(values));
        }
        RegCloseKey(key);
        if (!SystemParametersInfoA(SPI_SETMOUSE, 0, values, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) ||
            !SystemParametersInfoA(SPI_SETMOUSESPEED, 0, (PVOID)(INT_PTR)values[3],
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) return 11;
        printf("Pointer thresholds %d,%d, acceleration %d, speed %d\n",
               values[0], values[1], values[2], values[3]);
        return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "click")) {
        POINT target_pt = { atoi(argv[2]), atoi(argv[3]) };
        HWND target = WindowFromPoint(target_pt);
        char classname[64] = {0};
        GetClassNameA(target, classname, sizeof(classname));
        if (!strcmp(classname, "Button")) {
            PostMessageA(target, BM_CLICK, 0, 0);
            return 0;
        }
        if (!SetCursorPos(atoi(argv[2]), atoi(argv[3]))) return 6;
        Sleep(150);
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        Sleep(80);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        return 0;
    }
    if (!SystemParametersInfoA(SPI_GETMOUSETRAILS, 0, &trails, 0)) return 1;
    if (argc > 1 && (!strcmp(argv[1], "software") || !strcmp(argv[1], "restore"))) {
        HKEY key;
        DWORD saved = trails, size = sizeof(saved), type = 0;
        if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\SharedGL\\MouseFix", 0,
                NULL, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &key, NULL) != ERROR_SUCCESS)
            return 2;
        LONG found = RegQueryValueExA(key, "OriginalMouseTrails", NULL, &type,
                                     (BYTE *)&saved, &size);
        UINT target;
        if (!strcmp(argv[1], "software")) {
            if (found == ERROR_FILE_NOT_FOUND) {
                saved = trails;
                if (RegSetValueExA(key, "OriginalMouseTrails", 0, REG_DWORD,
                                  (BYTE *)&saved, sizeof(saved)) != ERROR_SUCCESS) {
                    RegCloseKey(key); return 3;
                }
            } else if (found != ERROR_SUCCESS || type != REG_DWORD || size != sizeof(saved)) {
                RegCloseKey(key); return 4;
            }
            target = 2;
        } else {
            if (found != ERROR_SUCCESS || type != REG_DWORD || size != sizeof(saved)) {
                puts("No saved setting; nothing changed."); RegCloseKey(key); return 4;
            }
            target = saved;
        }
        RegCloseKey(key);
        if (!SystemParametersInfoA(SPI_SETMOUSETRAILS, target, NULL,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) return 5;
        printf("Mouse trails: %u -> %u (original saved for restore).\n", trails, target);
    } else if (argc > 1 && strcmp(argv[1], "status")) {
        puts("Usage: sglmouse software | restore | linear | restore-linear | status | click X Y"); return 2;
    }
    SystemParametersInfoA(SPI_GETMOUSETRAILS, 0, &trails, 0);
    GetCursorPos(&pt);
    printf("Desktop %dx%d, cursor %ld,%ld, trails %u\n",
           GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
           pt.x, pt.y, trails);
    return 0;
}

/*
 * Spinning gears, written for this project: the classic scene of three
 * meshing gears drawn with OpenGL 1.1 display lists, plus an FPS report.
 *
 * Usage: sglgears [-t SECONDS] [-w WIDTH] [-h HEIGHT]
 * The report goes to stdout and the title bar. -t exits on its own.
 */
#include <windows.h>
#include <GL/gl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265358979f

static void gear(float inner, float outer, float width, int teeth, float depth)
{
    float r0 = inner, r1 = outer - depth / 2, r2 = outer + depth / 2;
    float da = 2 * PI / teeth / 4;
    float half = width * 0.5f;
    int i;

    glShadeModel(GL_FLAT);
    glNormal3f(0, 0, 1);
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= teeth; i++) {
        float a = i * 2 * PI / teeth;
        glVertex3f(r0 * cosf(a), r0 * sinf(a), half);
        glVertex3f(r1 * cosf(a), r1 * sinf(a), half);
        if (i < teeth) {
            glVertex3f(r0 * cosf(a), r0 * sinf(a), half);
            glVertex3f(r1 * cosf(a + 3 * da), r1 * sinf(a + 3 * da), half);
        }
    }
    glEnd();

    glBegin(GL_QUADS);
    for (i = 0; i < teeth; i++) {
        float a = i * 2 * PI / teeth;
        glVertex3f(r1 * cosf(a), r1 * sinf(a), half);
        glVertex3f(r2 * cosf(a + da), r2 * sinf(a + da), half);
        glVertex3f(r2 * cosf(a + 2 * da), r2 * sinf(a + 2 * da), half);
        glVertex3f(r1 * cosf(a + 3 * da), r1 * sinf(a + 3 * da), half);
    }
    glEnd();

    glNormal3f(0, 0, -1);
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= teeth; i++) {
        float a = i * 2 * PI / teeth;
        glVertex3f(r1 * cosf(a), r1 * sinf(a), -half);
        glVertex3f(r0 * cosf(a), r0 * sinf(a), -half);
        if (i < teeth) {
            glVertex3f(r1 * cosf(a + 3 * da), r1 * sinf(a + 3 * da), -half);
            glVertex3f(r0 * cosf(a), r0 * sinf(a), -half);
        }
    }
    glEnd();

    glBegin(GL_QUADS);
    for (i = 0; i < teeth; i++) {
        float a = i * 2 * PI / teeth;
        glVertex3f(r1 * cosf(a + 3 * da), r1 * sinf(a + 3 * da), -half);
        glVertex3f(r2 * cosf(a + 2 * da), r2 * sinf(a + 2 * da), -half);
        glVertex3f(r2 * cosf(a + da), r2 * sinf(a + da), -half);
        glVertex3f(r1 * cosf(a), r1 * sinf(a), -half);
    }
    glEnd();

    glBegin(GL_QUAD_STRIP);
    for (i = 0; i < teeth; i++) {
        float a = i * 2 * PI / teeth;
        float u, v, len;
        glVertex3f(r1 * cosf(a), r1 * sinf(a), half);
        glVertex3f(r1 * cosf(a), r1 * sinf(a), -half);
        u = r2 * cosf(a + da) - r1 * cosf(a);
        v = r2 * sinf(a + da) - r1 * sinf(a);
        len = sqrtf(u * u + v * v);
        glNormal3f(v / len, -u / len, 0);
        glVertex3f(r2 * cosf(a + da), r2 * sinf(a + da), half);
        glVertex3f(r2 * cosf(a + da), r2 * sinf(a + da), -half);
        glNormal3f(cosf(a), sinf(a), 0);
        glVertex3f(r2 * cosf(a + 2 * da), r2 * sinf(a + 2 * da), half);
        glVertex3f(r2 * cosf(a + 2 * da), r2 * sinf(a + 2 * da), -half);
        u = r1 * cosf(a + 3 * da) - r2 * cosf(a + 2 * da);
        v = r1 * sinf(a + 3 * da) - r2 * sinf(a + 2 * da);
        glNormal3f(v, -u, 0);
        glVertex3f(r1 * cosf(a + 3 * da), r1 * sinf(a + 3 * da), half);
        glVertex3f(r1 * cosf(a + 3 * da), r1 * sinf(a + 3 * da), -half);
        glNormal3f(cosf(a), sinf(a), 0);
    }
    glVertex3f(r1, 0, half);
    glVertex3f(r1, 0, -half);
    glEnd();

    glShadeModel(GL_SMOOTH);
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= teeth; i++) {
        float a = i * 2 * PI / teeth;
        glNormal3f(-cosf(a), -sinf(a), 0);
        glVertex3f(r0 * cosf(a), r0 * sinf(a), -half);
        glVertex3f(r0 * cosf(a), r0 * sinf(a), half);
    }
    glEnd();
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case WM_SIZE:
        if (LOWORD(lparam) && HIWORD(lparam) && wglGetCurrentContext()) {
            double aspect = (double)HIWORD(lparam) / LOWORD(lparam);
            glViewport(0, 0, LOWORD(lparam), HIWORD(lparam));
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glFrustum(-1, 1, -aspect, aspect, 5, 60);
            glMatrixMode(GL_MODELVIEW);
        }
        return 0;
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE)
            DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, message, wparam, lparam);
}

int main(int argc, char **argv)
{
    static const float light_pos[4] = { 5, 5, 10, 0 };
    static const float red[4] = { 0.8f, 0.1f, 0.0f, 1 };
    static const float green[4] = { 0.0f, 0.8f, 0.2f, 1 };
    static const float blue[4] = { 0.2f, 0.2f, 1.0f, 1 };
    int limit = 0, width = 300, height = 300, i;
    PIXELFORMATDESCRIPTOR pfd;
    WNDCLASSA wc;
    RECT rect;
    HWND hwnd;
    HDC hdc;
    HGLRC context;
    GLuint lists;
    int format, frames = 0;
    LARGE_INTEGER freq, start, last, now;
    float angle = 0;
    MSG msg;

    for (i = 1; i + 1 < argc; i += 2) {
        if (!strcmp(argv[i], "-t")) limit = atoi(argv[i + 1]);
        else if (!strcmp(argv[i], "-w")) width = atoi(argv[i + 1]);
        else if (!strcmp(argv[i], "-h")) height = atoi(argv[i + 1]);
    }
    setvbuf(stdout, NULL, _IONBF, 0);

    memset(&wc, 0, sizeof(wc));
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = window_proc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wc.lpszClassName = "sglgears";
    RegisterClassA(&wc);

    SetRect(&rect, 0, 0, width, height);
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    hwnd = CreateWindowA("sglgears", "sglgears", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                         60, 60, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, wc.hInstance, NULL);
    hdc = GetDC(hwnd);

    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;

    format = ChoosePixelFormat(hdc, &pfd);
    if (format == 0 || !SetPixelFormat(hdc, format, &pfd)) {
        printf("FAILED: pixel format, error %lu\n", GetLastError());
        return 2;
    }
    context = wglCreateContext(hdc);
    if (context == NULL || !wglMakeCurrent(hdc, context)) {
        printf("FAILED: context, error %lu\n", GetLastError());
        return 3;
    }

    printf("GL_VENDOR   : %s\n", glGetString(GL_VENDOR));
    printf("GL_RENDERER : %s\n", glGetString(GL_RENDERER));
    printf("GL_VERSION  : %s\n", glGetString(GL_VERSION));

    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);

    lists = glGenLists(3);
    glNewList(lists, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
    gear(1.0f, 4.0f, 1.0f, 20, 0.7f);
    glEndList();
    glNewList(lists + 1, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
    gear(0.5f, 2.0f, 2.0f, 10, 0.7f);
    glEndList();
    glNewList(lists + 2, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
    gear(1.3f, 2.0f, 0.5f, 10, 0.7f);
    glEndList();

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    window_proc(hwnd, WM_SIZE, 0, MAKELPARAM(width, height));

    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    last = start;

    for (;;) {
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                goto done;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        QueryPerformanceCounter(&now);
        angle = (float)((double)(now.QuadPart - start.QuadPart) / freq.QuadPart) * 70.0f;

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();
        glTranslatef(0, 0, -40);
        glPushMatrix();
        glRotatef(20, 1, 0, 0);
        glRotatef(30, 0, 1, 0);

        glPushMatrix();
        glTranslatef(-3, -2, 0);
        glRotatef(angle, 0, 0, 1);
        glCallList(lists);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(3.1f, -2, 0);
        glRotatef(-2 * angle - 9, 0, 0, 1);
        glCallList(lists + 1);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(-3.1f, 4.2f, 0);
        glRotatef(-2 * angle - 25, 0, 0, 1);
        glCallList(lists + 2);
        glPopMatrix();

        glPopMatrix();
        SwapBuffers(hdc);
        frames++;

        QueryPerformanceCounter(&now);
        if (now.QuadPart - last.QuadPart >= freq.QuadPart * 5) {
            char title[64];
            double fps = frames * (double)freq.QuadPart / (now.QuadPart - last.QuadPart);
            printf("%d frames, %.1f FPS\n", frames, fps);
            sprintf(title, "sglgears: %.1f FPS", fps);
            SetWindowTextA(hwnd, title);
            frames = 0;
            last = now;
            if (limit && now.QuadPart - start.QuadPart >= (LONGLONG)freq.QuadPart * limit)
                DestroyWindow(hwnd);
        }
    }

done:
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(context);
    printf("Exit OK\n");
    return 0;
}

#include <windows.h>

int runStudentScoreSystem(void);
int runContactSystem(void);
int runCampusDeliverySystem(void);

#define IDC_STUDENT_BTN 101
#define IDC_CONTACT_BTN 102
#define IDC_CAMPUS_BTN 103
#define IDC_EXIT_BTN 104

static HFONT g_mainFont = NULL;
static HBRUSH g_mainBrush = NULL;

static void createMainControls(HWND hwnd);
static void setChildFonts(HWND hwnd);
static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    WNDCLASSA wc;
    HWND hwnd;
    MSG msg;

    (void)hPrevInstance;
    (void)lpCmdLine;

    g_mainBrush = CreateSolidBrush(RGB(246, 250, 255));
    g_mainFont = CreateFontA(
        18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "DataStructureMainWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = g_mainBrush;

    RegisterClassA(&wc);

    hwnd = CreateWindowExA(
        0,
        "DataStructureMainWindow",
        "Data Structure Course Design",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        640,
        430,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (hwnd == NULL) {
        MessageBoxA(NULL, "Unable to create main window.", "Error", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    if (g_mainFont != NULL) {
        DeleteObject(g_mainFont);
        g_mainFont = NULL;
    }
    if (g_mainBrush != NULL) {
        DeleteObject(g_mainBrush);
        g_mainBrush = NULL;
    }

    return (int)msg.wParam;
}

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    (void)lParam;

    switch (msg) {
    case WM_CREATE:
        createMainControls(hwnd);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_STUDENT_BTN) {
            runStudentScoreSystem();
        }
        else if (LOWORD(wParam) == IDC_CONTACT_BTN) {
            runContactSystem();
        }
        else if (LOWORD(wParam) == IDC_CAMPUS_BTN) {
            runCampusDeliverySystem();
        }
        else if (LOWORD(wParam) == IDC_EXIT_BTN) {
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_CTLCOLORSTATIC:
        SetBkColor((HDC)wParam, RGB(246, 250, 255));
        SetTextColor((HDC)wParam, RGB(31, 41, 55));
        return (LRESULT)g_mainBrush;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void createMainControls(HWND hwnd)
{
    CreateWindowExA(0, "STATIC", "Data Structure Course Design",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        90, 34, 440, 34, hwnd, NULL, NULL, NULL);

    CreateWindowExA(0, "STATIC", "Choose one interactive Win32 system:",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        90, 78, 440, 28, hwnd, NULL, NULL, NULL);

    CreateWindowExA(0, "BUTTON", "Student Score Management",
        WS_CHILD | WS_VISIBLE,
        170, 130, 280, 44, hwnd, (HMENU)IDC_STUDENT_BTN, NULL, NULL);

    CreateWindowExA(0, "BUTTON", "Simple Contact Management",
        WS_CHILD | WS_VISIBLE,
        170, 190, 280, 44, hwnd, (HMENU)IDC_CONTACT_BTN, NULL, NULL);

    CreateWindowExA(0, "BUTTON", "Campus Delivery Path Planning",
        WS_CHILD | WS_VISIBLE,
        170, 250, 280, 44, hwnd, (HMENU)IDC_CAMPUS_BTN, NULL, NULL);

    CreateWindowExA(0, "BUTTON", "Exit",
        WS_CHILD | WS_VISIBLE,
        250, 318, 120, 38, hwnd, (HMENU)IDC_EXIT_BTN, NULL, NULL);

    setChildFonts(hwnd);
}

static void setChildFonts(HWND hwnd)
{
    HWND child;

    if (g_mainFont == NULL) {
        return;
    }

    child = GetWindow(hwnd, GW_CHILD);
    while (child != NULL) {
        SendMessageA(child, WM_SETFONT, (WPARAM)g_mainFont, TRUE);
        child = GetWindow(child, GW_HWNDNEXT);
    }
}

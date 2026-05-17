#define UNICODE
#define _UNICODE
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commdlg.h>

#define BAR_WIDTH       90
#define BAR_HEIGHT      450
#define SLOT_COUNT      6
#define ACTIVE_DURATION 10000

#define WM_TRAYICON     (WM_USER + 1)
#define TIMER_ID        101
#define TRAY_ICON_ID    1

HWND g_hWnd       = NULL;
HWND g_hLeftDock  = NULL;
HWND g_hRightDock = NULL;
NOTIFYICONDATA g_nid = {};
HINSTANCE g_hInst = NULL;

wchar_t g_configPath[MAX_PATH];
wchar_t g_leftSlots[SLOT_COUNT][MAX_PATH];
wchar_t g_rightSlots[SLOT_COUNT][MAX_PATH];

void LoadConfig() {
    GetModuleFileNameW(NULL, g_configPath, MAX_PATH);
    wchar_t* slash = wcsrchr(g_configPath, L'\\');
    if (slash) *(slash + 1) = L'\0';
    wcscat_s(g_configPath, L"config.ini");

    for (int i = 0; i < SLOT_COUNT; i++) {
        wchar_t key[16];
        wsprintf(key, L"Slot%d", i + 1);
        GetPrivateProfileStringW(L"LeftMonitorSlots",  key, L"EMPTY", g_leftSlots[i],  MAX_PATH, g_configPath);
        GetPrivateProfileStringW(L"RightMonitorSlots", key, L"EMPTY", g_rightSlots[i], MAX_PATH, g_configPath);
    }
}

void LaunchSlot(const wchar_t* path) {
    if (wcscmp(path, L"EMPTY") == 0 || wcslen(path) == 0) return;
    ShellExecuteW(NULL, L"open", path, NULL, NULL, SW_SHOWNORMAL);
}

RECT GetPrimaryMonitorRect() {
    // Primary monitor always contains (0,0) in Windows virtual screen space
    HMONITOR hMon = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMon, &mi);
    return mi.rcMonitor;
}

void ShowHotbars(HWND hWnd) {
    RECT mon = GetPrimaryMonitorRect();
    int monH  = mon.bottom - mon.top;
    int centY = mon.top + (monH - BAR_HEIGHT) / 2;

    SetWindowPos(g_hLeftDock,  HWND_TOPMOST, mon.left,              centY, BAR_WIDTH, BAR_HEIGHT, SWP_NOACTIVATE);
    SetWindowPos(g_hRightDock, HWND_TOPMOST, mon.right - BAR_WIDTH, centY, BAR_WIDTH, BAR_HEIGHT, SWP_NOACTIVATE);

    ShowWindow(g_hLeftDock,  SW_SHOWNOACTIVATE);
    ShowWindow(g_hRightDock, SW_SHOWNOACTIVATE);
    InvalidateRect(g_hLeftDock,  NULL, TRUE);
    InvalidateRect(g_hRightDock, NULL, TRUE);

    for (int i = 1; i <= SLOT_COUNT; i++) {
        RegisterHotKey(hWnd, i,            0,       '0' + i);
        RegisterHotKey(hWnd, i + SLOT_COUNT, MOD_ALT, '0' + i);
    }
    SetTimer(hWnd, TIMER_ID, ACTIVE_DURATION, NULL);
}

void HideHotbars(HWND hWnd) {
    KillTimer(hWnd, TIMER_ID);
    ShowWindow(g_hLeftDock,  SW_HIDE);
    ShowWindow(g_hRightDock, SW_HIDE);
    for (int i = 1; i <= SLOT_COUNT * 2; i++) UnregisterHotKey(hWnd, i);
}

void PaintDock(HWND hWnd, bool isRight) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    RECT rc;
    GetClientRect(hWnd, &rc);

    HBRUSH bgBrush = CreateSolidBrush(RGB(18, 18, 28));
    FillRect(hdc, &rc, bgBrush);
    DeleteObject(bgBrush);

    int slotH = BAR_HEIGHT / SLOT_COUNT;
    SetBkMode(hdc, TRANSPARENT);

    HFONT hFontKey  = CreateFontW(15, 0, 0, 0, FW_BOLD,    FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT hFontName = CreateFontW(13, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

    for (int i = 0; i < SLOT_COUNT; i++) {
        const wchar_t* path = isRight ? g_rightSlots[i] : g_leftSlots[i];
        bool empty = (wcscmp(path, L"EMPTY") == 0 || wcslen(path) == 0);

        // Slot divider
        if (i > 0) {
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(40, 40, 60));
            HPEN old = (HPEN)SelectObject(hdc, pen);
            MoveToEx(hdc, 6, i * slotH, NULL);
            LineTo(hdc, BAR_WIDTH - 6, i * slotH);
            SelectObject(hdc, old);
            DeleteObject(pen);
        }

        // Key hint
        wchar_t keyLabel[16];
        if (isRight) wsprintf(keyLabel, L"A+%d", i + 1);
        else         wsprintf(keyLabel, L"%d", i + 1);

        SelectObject(hdc, hFontKey);
        SetTextColor(hdc, empty ? RGB(60, 60, 80) : RGB(80, 180, 255));
        RECT keyRc = { 0, i * slotH + 5, BAR_WIDTH, i * slotH + 22 };
        DrawTextW(hdc, keyLabel, -1, &keyRc, DT_CENTER | DT_SINGLELINE);

        // App/URL name
        if (!empty) {
            const wchar_t* name = wcsrchr(path, L'\\');
            if (!name) name = wcsrchr(path, L'/');
            if (name) name++; else name = path;

            wchar_t shortName[MAX_PATH];
            wcsncpy_s(shortName, name, MAX_PATH - 1);
            wchar_t* dot = wcsrchr(shortName, L'.');
            if (dot && (_wcsicmp(dot, L".exe") == 0 || _wcsicmp(dot, L".lnk") == 0))
                *dot = L'\0';

            SelectObject(hdc, hFontName);
            SetTextColor(hdc, RGB(190, 190, 210));
            RECT nameRc = { 3, i * slotH + 22, BAR_WIDTH - 3, (i + 1) * slotH - 4 };
            DrawTextW(hdc, shortName, -1, &nameRc, DT_CENTER | DT_WORDBREAK | DT_END_ELLIPSIS);
        }
    }

    DeleteObject(hFontKey);
    DeleteObject(hFontName);
    EndPaint(hWnd, &ps);
}

LRESULT CALLBACK DockWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT:
        PaintDock(hWnd, GetWindowLongPtr(hWnd, GWLP_USERDATA) == 1);
        return 0;

    case WM_RBUTTONDOWN: {
        int slot = GET_Y_LPARAM(lParam) / (BAR_HEIGHT / SLOT_COUNT);
        if (slot < 0 || slot >= SLOT_COUNT) break;
        bool isRight = (GetWindowLongPtr(hWnd, GWLP_USERDATA) == 1);

        OPENFILENAMEW ofn = {};
        wchar_t szFile[MAX_PATH] = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner   = hWnd;
        ofn.lpstrFile   = szFile;
        ofn.nMaxFile    = MAX_PATH;
        ofn.lpstrFilter = L"Executables\0*.exe\0All Files\0*.*\0";
        ofn.Flags       = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameW(&ofn)) {
            const wchar_t* section = isRight ? L"RightMonitorSlots" : L"LeftMonitorSlots";
            wchar_t key[16];
            wsprintf(key, L"Slot%d", slot + 1);
            WritePrivateProfileStringW(section, key, szFile, g_configPath);
            LoadConfig();
            InvalidateRect(hWnd, NULL, TRUE);
        }
        return 0;
    }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TRAYICON:
        if (lParam == WM_LBUTTONDOWN) ShowHotbars(hWnd);
        break;

    case WM_HOTKEY: {
        int id = (int)wParam;
        if (id >= 1 && id <= SLOT_COUNT)
            LaunchSlot(g_leftSlots[id - 1]);
        else if (id > SLOT_COUNT && id <= SLOT_COUNT * 2)
            LaunchSlot(g_rightSlots[id - SLOT_COUNT - 1]);
        HideHotbars(hWnd);
        break;
    }

    case WM_TIMER:
        if (wParam == TIMER_ID) HideHotbars(hWnd);
        break;

    case WM_DESTROY:
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    g_hInst = hInstance;
    LoadConfig();

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"HotBoard1701_Main";
    RegisterClassExW(&wc);

    WNDCLASSEXW dc = {};
    dc.cbSize        = sizeof(dc);
    dc.lpfnWndProc   = DockWndProc;
    dc.hInstance     = hInstance;
    dc.hbrBackground = CreateSolidBrush(RGB(18, 18, 28));
    dc.lpszClassName = L"HotBoard1701_Dock";
    RegisterClassExW(&dc);

    g_hWnd = CreateWindowExW(0, L"HotBoard1701_Main", L"HotBoard1701",
        WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    ShowWindow(g_hWnd, SW_HIDE);

    g_hLeftDock = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
        L"HotBoard1701_Dock", NULL, WS_POPUP,
        0, 0, BAR_WIDTH, BAR_HEIGHT, NULL, NULL, hInstance, NULL);
    SetWindowLongPtr(g_hLeftDock, GWLP_USERDATA, 0);

    g_hRightDock = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
        L"HotBoard1701_Dock", NULL, WS_POPUP,
        0, 0, BAR_WIDTH, BAR_HEIGHT, NULL, NULL, hInstance, NULL);
    SetWindowLongPtr(g_hRightDock, GWLP_USERDATA, 1);

    g_nid.cbSize          = sizeof(g_nid);
    g_nid.hWnd            = g_hWnd;
    g_nid.uID             = TRAY_ICON_ID;
    g_nid.uFlags          = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon           = LoadIcon(NULL, IDI_APPLICATION);
    lstrcpyW(g_nid.szTip, L"HotBoard1701");
    Shell_NotifyIconW(NIM_ADD, &g_nid);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

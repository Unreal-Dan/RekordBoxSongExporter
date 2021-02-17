#include <Windows.h>
#include <uxtheme.h>
#include <CommCtrl.h>
#include <inttypes.h>
#include <CommCtrl.h>

#include <string>

#include "Injector.h"
#include "resource.h"

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "UxTheme.lib")

using namespace std;

struct version_path
{
    const char *name;
    const char *path;
};

// list of supported versions
version_path versions[] = {
    // Friendly name     Default installation path
    { "Rekordbox 6.5.0", "C:\\Program Files\\Pioneer\\rekordbox 6.5.0\\rekordbox.exe" },
    { "Rekordbox 5.8.5", "C:\\Program Files\\Pioneer\\rekordbox 5.8.5\\rekordbox.exe" },
};

// the number of versions in table above
#define NUM_VERSIONS    (sizeof(versions) / sizeof(versions[0]))

// module base
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
HINSTANCE imageBase = (HINSTANCE)&__ImageBase;

#define COMBOBOX_ID 1000
#define CHECKBOX_ID 1001
#define BUTTON_ID   1002
#define EDIT_ID     1003

HWND hwndComboBox;
HWND hwndCheckBox;
HWND hwndButton;
HWND hwndEdit;
HBRUSH back;
HBRUSH comboBack;

LRESULT CALLBACK comboProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubClass, DWORD_PTR)
{
    if (msg != WM_PAINT) {
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    COLORREF bkcolor = RGB(25, 25, 25);
    COLORREF textcolor = RGB(255, 255, 255);
    COLORREF arrowcolor = RGB(200, 200, 200);

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT rc;
    GetClientRect(hwnd, &rc);

    HBRUSH brush = CreateSolidBrush(bkcolor);
    HGDIOBJ oldbrush = SelectObject(hdc, brush);
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HGDIOBJ oldpen = SelectObject(hdc, pen);

    SelectObject(hdc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
    SetBkColor(hdc, bkcolor);
    SetTextColor(hdc, textcolor);

    // draw the two rectangles
    Rectangle(hdc, 0, 0, rc.right, rc.bottom);

    // need to redraw the text as long as the dropdown is open, win32 sucks
    int index = (int)SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    if (index >= 0) {
        size_t buflen = (size_t)SendMessage(hwnd, CB_GETLBTEXTLEN, index, 0);
        char *buf = new char[(buflen + 1)];
        SendMessage(hwnd, CB_GETLBTEXT, index, (LPARAM)buf);
        rc.left += 5;
        DrawText(hdc, buf, -1, &rc, DT_EDITCONTROL | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        delete[] buf;
    }

    Rectangle(hdc, rc.right - 25, rc.top + 2, rc.right - 24, rc.bottom - 2);

    DeleteObject(brush);
    DeleteObject(pen);

    pen = CreatePen(PS_SOLID, 1, arrowcolor);
    brush = CreateSolidBrush(arrowcolor);
    SelectObject(hdc, brush);
    SelectObject(hdc, pen);

    POINT vertices[] = { {123, 7}, {133, 7}, {128, 15} };
    SetPolyFillMode(hdc, ALTERNATE);
    Polygon(hdc, vertices, sizeof(vertices) / sizeof(vertices[0]));

    SelectObject(hdc, oldbrush);
    DeleteObject(brush);
    SelectObject(hdc, oldpen);
    DeleteObject(pen);

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void doCreate(HWND hwnd)
{
    // set icon
    HICON hIcon = LoadIcon(imageBase, MAKEINTRESOURCE(IDI_ICON1));
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

    back = CreateSolidBrush(0x282828);
    comboBack = CreateSolidBrush(0x191919);

    // create the path entry text box
    hwndEdit = CreateWindow(WC_EDIT, "PathEntry", 
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 
        12, 46, 376, 21, hwnd, (HMENU)EDIT_ID, NULL, NULL);

    // set the version path in the text box
    SetWindowText(hwndEdit, versions[0].path);

    // create the dropdown box
    hwndComboBox = CreateWindow(WC_COMBOBOX, "VersionSelect",
        CBS_SIMPLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
        12, 16, 140, 400, hwnd, (HMENU)COMBOBOX_ID, NULL, NULL);
    if (!hwnd) {
        MessageBox(NULL, "Failed to open window", "Error", 0);
        return;
    }
    // populate the dropdown box with versions
    for (size_t i = 0; i < NUM_VERSIONS; i++) {
        // Add string to combobox.
        SendMessage(hwndComboBox, CB_ADDSTRING, NULL, (LPARAM)versions[i].name);
    }
    // Send the CB_SETCURSEL message to display an initial item in the selection field  
    SendMessage(hwndComboBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

    // for overriding background colors of dropdown
    SetWindowSubclass(hwndComboBox, comboProc, 0, 0);

    // create the artist checkbox
    hwndCheckBox = CreateWindow(WC_BUTTON, "Include Artist", 
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        17, 80, 115, 16, hwnd, (HMENU)CHECKBOX_ID, NULL, NULL);

    // create the launch button
    hwndButton = CreateWindow(WC_BUTTON, "Launch",
        WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        290, 80, 100, 40, hwnd, (HMENU)BUTTON_ID, NULL, NULL);
}

void doDestroy(HWND hwnd)
{
    DeleteObject(back);
    DeleteObject(comboBack);
}

void doPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    FillRect(hdc, &ps.rcPaint, (HBRUSH)back);
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    LPDRAWITEMSTRUCT draw_item = NULL;
    char buf[2048] = {0};
    switch (uMsg) {
    case WM_DRAWITEM:
        draw_item = (LPDRAWITEMSTRUCT)lParam;
        break;
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) {
            if (LOWORD(wParam) == BUTTON_ID) {
                GetWindowText(hwndEdit, buf, sizeof(buf));
                int sel = (int)SendMessageW(hwndComboBox, CB_GETCURSEL, NULL, NULL);
                bool use_artist = (bool)SendMessage(hwndCheckBox, BM_GETCHECK, 0, 0);
                if (!inject(buf, versions[sel].name, use_artist)) {
                    string msg = string("Failed to inject into ") + buf;
                    MessageBox(NULL, msg.c_str(), "Error", 0);
                }
            }
        }
        if (LOWORD(wParam) == COMBOBOX_ID) {
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                int sel = (int)SendMessageW(hwndComboBox, CB_GETCURSEL, NULL, NULL);
                SetWindowText(hwndEdit, versions[sel].path);
            }
        }
        break;
    case WM_INITDIALOG:
        break;
    case WM_CREATE:
        doCreate(hwnd);
        break;
    case WM_DESTROY:
        doDestroy(hwnd);
        PostQuitMessage(0);
        break;
    case WM_PAINT:
        doPaint(hwnd);
        return 0;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
        hdc = (HDC)wParam;
        SetBkColor(hdc, RGB(25, 25, 25));
        SetTextColor(hdc, RGB(220, 220, 220));
        if (lParam == (LPARAM)hwndButton) {
            return (LRESULT)comboBack;
        }

        if (lParam == (LPARAM)hwndComboBox) {
            return (LRESULT)comboBack;
        }
        if (lParam == (LPARAM)hwndCheckBox) {
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)back;
        }
        return (LRESULT)comboBack;
    default:
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
    WNDCLASS wc;
    HWND hwnd;
    MSG msg;

    memset(&msg, 0, sizeof(msg));
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "RekordBoxSongExporter";
    RegisterClass(&wc);

    RECT desktop;
    GetClientRect(GetDesktopWindow(), &desktop);

    // create the window
    hwnd = CreateWindow(wc.lpszClassName, "Rekordbox Song Exporter",
        WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 
        (desktop.right/2) - 240, (desktop.bottom/2) - 84, 
        420, 169, NULL, NULL, hInstance, NULL);
    if (!hwnd) {
        MessageBox(NULL, "Failed to open window", "Error", 0);
        return 0;
    }

    // main message loop
    ShowWindow(hwnd, nCmdShow);
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}

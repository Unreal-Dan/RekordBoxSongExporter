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
HBRUSH bkbrush;
HBRUSH bkbrush2;
HBRUSH arrowbrush;
HPEN borderpen;
HPEN arrowpen;

COLORREF bkcolor = RGB(40, 40, 40);
COLORREF bkcolor2 = RGB(25, 25, 25);
COLORREF textcolor = RGB(220, 220, 220);
COLORREF arrowcolor = RGB(200, 200, 200);

void drawComboText(HWND hwnd, HDC hdc, RECT rc)
{
    // select font and text color
    SelectObject(hdc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
    SetTextColor(hdc, textcolor);
    // need to redraw the text as long as the dropdown is open, win32 sucks
    int index = (int)SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    if (index >= 0) {
        size_t buflen = (size_t)SendMessage(hwnd, CB_GETLBTEXTLEN, index, 0);
        char *buf = new char[(buflen + 1)];
        SendMessage(hwnd, CB_GETLBTEXT, index, (LPARAM)buf);
        rc.left += 4;
        DrawText(hdc, buf, -1, &rc, DT_EDITCONTROL | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        delete[] buf;
    }
}

LRESULT CALLBACK comboProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubClass, DWORD_PTR)
{
    if (msg != WM_PAINT) {
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }
    // the vertices for the dropdown triangle
    static const POINT vertices[] = { 
        {122, 10}, {132, 10}, {127, 15} 
    };
    HGDIOBJ oldbrush;
    HGDIOBJ oldpen;
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rc;

    hdc = BeginPaint(hwnd, &ps);
    GetClientRect(hwnd, &rc);

    // set background brush and border pen
    oldbrush = SelectObject(hdc, bkbrush2);
    oldpen = SelectObject(hdc, borderpen);
    // set background color
    SetBkColor(hdc, bkcolor2);

    // draw the two rectangles
    Rectangle(hdc, 0, 0, rc.right, rc.bottom);
    // redraw the text 
    drawComboText(hwnd, hdc, rc);
    // draw the box around the dropdown button part
    Rectangle(hdc, rc.right - 25, rc.top + 2, rc.right - 24, rc.bottom - 2);

    // select pen and brush for drawing down arrow
    SelectObject(hdc, arrowbrush);
    SelectObject(hdc, arrowpen);
    // draw the down arrow
    SetPolyFillMode(hdc, ALTERNATE);
    Polygon(hdc, vertices, sizeof(vertices) / sizeof(vertices[0]));

    // restore old brush and pen
    SelectObject(hdc, oldbrush);
    SelectObject(hdc, oldpen);

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void doCreate(HWND hwnd)
{
    // set icon
    HICON hIcon = LoadIcon(imageBase, MAKEINTRESOURCE(IDI_ICON1));
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

    bkbrush = CreateSolidBrush(bkcolor);
    bkbrush2 = CreateSolidBrush(bkcolor2);
    arrowbrush = CreateSolidBrush(arrowcolor);
    borderpen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    arrowpen = CreatePen(PS_SOLID, 1, arrowcolor);

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
    DeleteObject(bkbrush);
    DeleteObject(bkbrush2);
    DeleteObject(arrowbrush);
    DeleteObject(borderpen);
    DeleteObject(arrowpen);
}

void doPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    FillRect(hdc, &ps.rcPaint, (HBRUSH)bkbrush);
    EndPaint(hwnd, &ps);
}

LRESULT doButtonPaint(WPARAM wParam, LPARAM lParam)
{
    HDC hdc = (HDC)wParam;
    SetBkColor(hdc, bkcolor2);
    SetTextColor(hdc, textcolor);
    if (lParam == (LPARAM)hwndButton) {
        return (LRESULT)bkbrush2;
    }

    if (lParam == (LPARAM)hwndComboBox) {
        return (LRESULT)bkbrush2;
    }
    if (lParam == (LPARAM)hwndCheckBox) {
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)bkbrush;
    }
    return (LRESULT)bkbrush2;
}

void doInject()
{
    char buf[2048] = {0};
    GetWindowText(hwndEdit, buf, sizeof(buf));
    int sel = (int)SendMessageW(hwndComboBox, CB_GETCURSEL, NULL, NULL);
    bool use_artist = (bool)SendMessage(hwndCheckBox, BM_GETCHECK, 0, 0);
    if (!inject(buf, versions[sel].name, use_artist)) {
        string msg = string("Failed to inject into ") + buf;
        MessageBox(NULL, msg.c_str(), "Error", 0);
    }
}

void updatePathEditBox()
{
    int sel = (int)SendMessageW(hwndComboBox, CB_GETCURSEL, NULL, NULL);
    SetWindowText(hwndEdit, versions[sel].path);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_COMMAND:
        // when button clicked inject the module
        if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == BUTTON_ID) {
            doInject();
            break;
        }
        // when combobox changes update the text box
        if (LOWORD(wParam) == COMBOBOX_ID && HIWORD(wParam) == CBN_SELCHANGE) {
            updatePathEditBox();
            break;
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
        return doButtonPaint(wParam, lParam);
    default:
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
    RECT desktop;
    WNDCLASS wc;
    HWND hwnd;
    MSG msg;

    // class registration
    memset(&msg, 0, sizeof(msg));
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "RekordBoxSongExporter";
    RegisterClass(&wc);

    // get desktop rect so we can center the window
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

#include <Windows.h>
#include <Windowsx.h>
#include <uxtheme.h>
#include <CommCtrl.h>
#include <inttypes.h>
#include <CommCtrl.h>

#include <vector>
#include <string>

#include "OutputFiles.h"
#include "Injector.h"
#include "resource.h"
#include "Config.h"

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "UxTheme.lib")
#pragma comment(lib, "Winmm.lib")

using namespace std;

struct version_path
{
    const char *name;
    const char *path;
};

// list of supported versions
version_path versions[] = {
    // Friendly name     Default installation path
    { "Rekordbox 6.5.1", "C:\\Program Files\\Pioneer\\rekordbox 6.5.1\\rekordbox.exe" },
    { "Rekordbox 6.5.0", "C:\\Program Files\\Pioneer\\rekordbox 6.5.0\\rekordbox.exe" },
    { "Rekordbox 5.8.5", "C:\\Program Files\\Pioneer\\rekordbox 5.8.5\\rekordbox.exe" },
};

// the number of versions in table above
#define NUM_VERSIONS    (sizeof(versions) / sizeof(versions[0]))

// version select combobox
HWND hwndVersionCombo;
#define VERSION_COMBO_ID        1000

// server checkbox
HWND hwndServerCheck;
#define SERVER_CHECK_ID         1001

// server ip edit box
HWND hwndServerEdit;
#define SERVER_EDIT_ID          1002

// rekordbox path textbox
HWND hwndPathEdit;
#define PATH_EDIT_ID            1003

// the listbox of output files
HWND hwndOutfilesList;
#define OUTFILES_LIST_ID        1004

// the delete output file button
HWND hwndDelFileButton;
#define DEL_BUTTON_ID           1005

// the add output file button
HWND hwndAddFileButton;
#define ADD_BUTTON_ID           1006

// output filename edit textbox
HWND hwndOutfileNameEdit;
#define OUTFILENAME_EDIT_ID     1007

// output file format edit textbox
HWND hwndOutfileFormatEdit;
#define OUTFILEFORMAT_EDIT_ID   1008

// output file replace mode radio
HWND hwndReplaceModeRadio;
#define REPLACE_RADIO_ID        1009

// output file append mode radio
HWND hwndAppendModeRadio;
#define APPEND_RADIO_ID         1010

// output file prepend mode radio
HWND hwndPrependModeRadio;
#define PREPEND_RADIO_ID        1011

// "Offset:" label (don't need ID)
HWND hwndOffsetLabel;
// Offset edit box
HWND hwndOffsetEdit;
#define OFFSET_EDIT_ID          1012

// "Max Lines:" label (don't need ID)
HWND hwndMaxLinesLabel;
// Max lines edit box
HWND hwndMaxLinesEdit;
#define MAXLINES_EDIT_ID        1013

// launch button
HWND hwndButton;
#define LAUNCH_BUTTON_ID        1014


// background color brush 1
HBRUSH bkbrush;
// background color brush 2
HBRUSH bkbrush2;
// brush for down arrow in dropdown
HBRUSH arrowbrush;
// pen for drawing border on dropdown
HPEN borderpen;
// pen for drawing arrow in dropdown
HPEN arrowpen;

// background color 1
COLORREF bkcolor = RGB(40, 40, 40);
// background color 2
COLORREF bkcolor2 = RGB(25, 25, 25);
// text color
COLORREF textcolor = RGB(220, 220, 220);
// down arrow color in dropdown
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

    // create the version select dropdown box
    hwndVersionCombo = CreateWindow(WC_COMBOBOX, "VersionSelect",
        CBS_SIMPLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_TABSTOP,
        12, 12, 140, 400, hwnd, (HMENU)VERSION_COMBO_ID, NULL, NULL);
    size_t cur_sel = 0;
    string conf_version = conf_load_version();
    // populate the dropdown box with versions
    for (size_t i = 0; i < NUM_VERSIONS; i++) {
        // Add string to combobox.
        SendMessage(hwndVersionCombo, CB_ADDSTRING, NULL, (LPARAM)versions[i].name);
        // the versions[i].name has the full "rekordbox 6.5.0" and we
        // just want to compare the version number so add 10
        if (conf_version == (versions[i].name + 10)) {
            cur_sel = i;
        }
    }
    // Send the CB_SETCURSEL message to display an initial item in the selection field  
    SendMessage(hwndVersionCombo, CB_SETCURSEL, (WPARAM)cur_sel, (LPARAM)0);
    // for overriding background colors of dropdown
    SetWindowSubclass(hwndVersionCombo, comboProc, 0, 0);

    // create the server checkbox and ip textbox
    hwndServerCheck = CreateWindow(WC_BUTTON, "Server", 
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_LEFTTEXT | WS_TABSTOP,
        198, 16, 65, 16, hwnd, (HMENU)SERVER_CHECK_ID, NULL, NULL);

    // whether to enable server and edit checkbox
    if (conf_load_use_server()) {
        SendMessage(hwndServerCheck, BM_SETCHECK, BST_CHECKED, 0);
    } else {
        EnableWindow(hwndServerEdit, false);
    }

    // create server ip entry
    string serverIp = conf_load_server_ip();
    hwndServerEdit  = CreateWindow(WC_EDIT, serverIp.c_str(), 
        WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP,
        270, 14, 120, 20, hwnd, (HMENU)SERVER_EDIT_ID, NULL, NULL);

    // create the install path entry text box
    hwndPathEdit = CreateWindow(WC_EDIT, "", 
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 
        12, 42, 378, 21, hwnd, (HMENU)PATH_EDIT_ID, NULL, NULL);

    // set the version path in the text box
    string path = conf_load_path();
    // default the path if there's no config file
    if (!path.length()) {
        path = versions[0].path;
    }
    SetWindowText(hwndPathEdit, path.c_str());

    //  end global configs
    // =============================
    //  begin output file configs

    // load the outputfiles from the config
    loadOutputFiles();

    // Create listbox of output files
    hwndOutfilesList = CreateWindow(WC_LISTBOX, NULL, 
        WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | WS_TABSTOP,
        12, 70, 140, 128, hwnd, (HMENU)OUTFILES_LIST_ID, NULL, NULL);

    // populate the listbox with names, when an item is
    // selected the rest of the fields will be populated
    for (int i = 0; i < outputFiles.size(); ++i) {
        ListBox_InsertString(hwndOutfilesList, i, outputFiles[i].name.c_str());
    }

    // Button to delete entries from listbox
    hwndDelFileButton = CreateWindow(WC_BUTTON, "Delete",
        WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP,
        12, 192, 69, 28, hwnd, (HMENU)DEL_BUTTON_ID, NULL, NULL);

    // Button to add new entries to listbox
    hwndAddFileButton = CreateWindow(WC_BUTTON, "Add",
        WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP,
        83, 192, 69, 28, hwnd, (HMENU)ADD_BUTTON_ID, NULL, NULL);

    // Output file name edit textbox
    hwndOutfileNameEdit = CreateWindow(WC_EDIT, "", 
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 
        160, 70, 230, 20, hwnd, (HMENU)OUTFILENAME_EDIT_ID, NULL, NULL);
    EnableWindow(hwndOutfileNameEdit, false);

    // limit the output file names to 64 letters each
    SendMessage(hwndOutfileNameEdit, EM_LIMITTEXT, (WPARAM)MAX_OUTFILE_NAME_LEN, NULL);

    // Output file format edit textbox
    hwndOutfileFormatEdit = CreateWindow(WC_EDIT, "", 
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 
        160, 96, 230, 20, hwnd, (HMENU)OUTFILEFORMAT_EDIT_ID, NULL, NULL);
    EnableWindow(hwndOutfileFormatEdit, false);

    // limit the output file formats to 512 letters each
    SendMessage(hwndOutfileFormatEdit, EM_LIMITTEXT, (WPARAM)512, NULL);

    // Output file replace mode radio
    hwndReplaceModeRadio = CreateWindow(WC_BUTTON, "Replace",
        WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, // begin group
        160, 120, 75, 24, hwnd, (HMENU)REPLACE_RADIO_ID, NULL, NULL);
    EnableWindow(hwndReplaceModeRadio, false);

    // Output file append mode radio
    hwndAppendModeRadio = CreateWindow(WC_BUTTON, "Append",
        WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
        240, 120, 70, 24, hwnd, (HMENU)APPEND_RADIO_ID, NULL, NULL);
    EnableWindow(hwndAppendModeRadio, false);

    // Output file prepend mode radio
    hwndPrependModeRadio = CreateWindow(WC_BUTTON, "Prepend",
        WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
        315, 120, 75, 24, hwnd, (HMENU)PREPEND_RADIO_ID, NULL, NULL);
    EnableWindow(hwndPrependModeRadio, false);

    // Create the offset label
    hwndOffsetLabel = CreateWindowEx(WS_EX_TRANSPARENT, WC_STATIC, "Offset:", 
        WS_CHILD | WS_VISIBLE | SS_LEFT | WS_SYSMENU, 
        162, 150, 46, 16, hwnd, NULL, NULL, NULL);

    // Create the offset text box
    hwndOffsetEdit = CreateWindow(WC_EDIT, "", 
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | WS_TABSTOP, 
        210, 148, 40, 20, hwnd, (HMENU)OFFSET_EDIT_ID, NULL, NULL);
    EnableWindow(hwndOffsetEdit, false);

    // Create the max lines label
    hwndMaxLinesLabel = CreateWindowEx(WS_EX_TRANSPARENT, WC_STATIC, "Max Lines:", 
        WS_CHILD | WS_VISIBLE | SS_LEFT | WS_SYSMENU, 
        272, 150, 76, 16, hwnd, NULL, NULL, NULL);

    // Create the max lines text box
    hwndMaxLinesEdit = CreateWindow(WC_EDIT, "", 
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | WS_TABSTOP, 
        350, 148, 40, 20, hwnd, (HMENU)MAXLINES_EDIT_ID, NULL, NULL);
    EnableWindow(hwndMaxLinesEdit, false);

    // create the launch button
    hwndButton = CreateWindow(WC_BUTTON, "Launch",
        WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP,
        160, 180, 230, 42, hwnd, (HMENU)LAUNCH_BUTTON_ID, NULL, NULL);

    // -----------------------------------------------------
    // after everything is created send a message to select 
    // the first entry in the listbox if there is one
    if (outputFiles.size() > 0) {
        ListBox_SetCurSel(hwndOutfilesList, 0);
        // SetCurSel won't trigger the LBN_SELCHANGE event so we
        // just send it ourselves to trigger the selection
        SendMessage(hwnd, WM_COMMAND, 
            MAKEWPARAM(OUTFILES_LIST_ID, LBN_SELCHANGE), 
            (LPARAM)hwndOutfilesList);
    }
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

    if (lParam == (LPARAM)hwndVersionCombo) {
        return (LRESULT)bkbrush2;
    }
    if (lParam == (LPARAM)hwndOffsetLabel || 
        lParam == (LPARAM)hwndMaxLinesLabel || 
        lParam == (LPARAM)hwndReplaceModeRadio ||
        lParam == (LPARAM)hwndAppendModeRadio ||
        lParam == (LPARAM)hwndPrependModeRadio ||
        lParam == (LPARAM)hwndServerCheck
        ) {
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)bkbrush;
    }
    return (LRESULT)bkbrush2;
}

// save the config file from current gui settings
void saveConfig()
{
    char buf[2048] = {0};

    // the version selection
    int sel = (int)SendMessageW(hwndVersionCombo, CB_GETCURSEL, NULL, NULL);
    // only save the part after the word "Rekordbox " (the version number)
    conf_save_version(versions[sel].name + sizeof("Rekordbox"));

    // the path
    GetWindowText(hwndPathEdit, buf, sizeof(buf));
    conf_save_path(buf);

    // the format
    //GetWindowText(hwndFmtEdit, buf, sizeof(buf));
    //conf_save_out_format(buf);

    // the num lines in cur tracks
    //GetWindowText(hwndCountEdit, buf, sizeof(buf));
    //conf_save_cur_tracks_count(buf);

    // whether server enabled
    conf_save_use_server((bool)SendMessage(hwndServerCheck, BM_GETCHECK, 0, 0));

    // save the server ip
    GetWindowText(hwndServerEdit, buf, sizeof(buf));
    conf_save_server_ip(buf);
}

// inject into the path specified in gui
void doInject()
{
    char buf[2048] = {0};
    GetWindowText(hwndPathEdit, buf, sizeof(buf));
    string rbox_path = buf;
    // inject the dll
    if (!inject(rbox_path)) {
        string msg = string("Failed to inject into ") + buf;
        MessageBox(NULL, msg.c_str(), "Error", 0);
    }
}

// update the path text box when the combo selection changes
void updatePathEditBox()
{
    int sel = (int)SendMessageW(hwndVersionCombo, CB_GETCURSEL, NULL, NULL);
    SetWindowText(hwndPathEdit, versions[sel].path);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_COMMAND:
        // when button clicked inject the module
        if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == LAUNCH_BUTTON_ID) {
            saveConfig();
            doInject();
            break;
        }
        if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == ADD_BUTTON_ID) {
            // add 1 to the current selection so that the new item appears
            // below the current selection -- unless the current selection
            // is -1 then it just adds to the bottom
            int sel = ListBox_GetCurSel(hwndOutfilesList);
            if (sel >= 0) {
                sel += 1;
            }
            outputFiles.push_back(output_file("OutputFile"));
            // TODO: generate new output file name
            ListBox_InsertString(hwndOutfilesList, sel, "OutputFile");
            ListBox_SetCurSel(hwndOutfilesList, ((sel == -1) ? 0 : sel));
            // SetCurSel won't trigger the LBN_SELCHANGE event so we
            // just send it ourselves to trigger the selection
            SendMessage(hwnd, WM_COMMAND,
                MAKEWPARAM(OUTFILES_LIST_ID, LBN_SELCHANGE),
                (LPARAM)hwndOutfilesList);
            break;
        }
        if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == DEL_BUTTON_ID) {
            // grab the currently selected entry, -1 means no selection
            int sel = ListBox_GetCurSel(hwndOutfilesList);
            if (sel == -1) {
                break;
            }
            // delete the entry at the selected spot
            ListBox_DeleteString(hwndOutfilesList, sel);
            // try to re-select the same spot
            ListBox_SetCurSel(hwndOutfilesList, sel);
            // if it didn't work
            if (ListBox_GetCurSel(hwndOutfilesList) == -1 && sel > 0) {
                // try to select the one above
                ListBox_SetCurSel(hwndOutfilesList, (sel > 0 ? sel - 1 : sel));
            }
            // SetCurSel won't trigger the LBN_SELCHANGE event so we
            // just send it ourselves to trigger the selection
            SendMessage(hwnd, WM_COMMAND,
                MAKEWPARAM(OUTFILES_LIST_ID, LBN_SELCHANGE),
                (LPARAM)hwndOutfilesList);
            break;
        }
        if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == OUTFILES_LIST_ID) {
            int sel = ListBox_GetCurSel(hwndOutfilesList);
            EnableWindow(hwndOutfileNameEdit, true);
            Edit_SetText(hwndOutfileNameEdit, outputFiles[sel].name.c_str());

            EnableWindow(hwndOutfileFormatEdit, true);
            Edit_SetText(hwndOutfileFormatEdit, outputFiles[sel].format.c_str());

            EnableWindow(hwndReplaceModeRadio, true);
            EnableWindow(hwndAppendModeRadio, true);
            EnableWindow(hwndPrependModeRadio, true);
            CheckRadioButton(hwnd, REPLACE_RADIO_ID, PREPEND_RADIO_ID, 
                REPLACE_RADIO_ID + (int)outputFiles[sel].mode);

            EnableWindow(hwndOffsetEdit, true);
            EnableWindow(hwndMaxLinesEdit, true);
            break;
        }
        if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == OUTFILENAME_EDIT_ID) {
            int sel = ListBox_GetCurSel(hwndOutfilesList);
            char name[MAX_OUTFILE_NAME_LEN] = {0};
            Edit_GetText(hwndOutfileNameEdit, name, sizeof(name) - 1);
            ListBox_DeleteString(hwndOutfilesList, sel);
            ListBox_InsertString(hwndOutfilesList, sel, name);
            ListBox_SetCurSel(hwndOutfilesList, sel);
            outputFiles[sel].name = name;
        }
        if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == OUTFILEFORMAT_EDIT_ID) {
            int sel = ListBox_GetCurSel(hwndOutfilesList);
            char format[MAX_OUTFILE_FORMAT_LEN] = {0};
            Edit_GetText(hwndOutfileFormatEdit, format, sizeof(format) - 1);
            outputFiles[sel].format = format;
        }
        if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == SERVER_CHECK_ID) {
            EnableWindow(hwndServerEdit, !IsWindowEnabled(hwndServerEdit));
            break;
        }
        // when combobox changes update the text box
        if (LOWORD(wParam) == VERSION_COMBO_ID && HIWORD(wParam) == CBN_SELCHANGE) {
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
    hwnd = CreateWindow(wc.lpszClassName, "Rekordbox Song Exporter " RBSE_VERSION,
        WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 
        (desktop.right/2) - 240, (desktop.bottom/2) - 84, 
        420, 269, NULL, NULL, hInstance, NULL);
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

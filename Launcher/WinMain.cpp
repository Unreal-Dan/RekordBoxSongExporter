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

// describes a supported version and it's path
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
HWND hwndLaunchButton;
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

// prototypes of static functions
static string get_window_text(HWND hwnd);
static void draw_combo_text(HWND hwnd, HDC hdc, RECT rc);
static LRESULT CALLBACK version_combo_subproc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubClass, DWORD_PTR);
static void do_create(HWND hwnd);
static void do_destroy(HWND hwnd);
static void do_paint(HWND hwnd);
static LRESULT do_button_paint(WPARAM wParam, LPARAM lParam);
static void do_save_config();
static void do_inject();
static void handle_click(HWND hwnd, WPARAM wParam, LPARAM lParam);
static void handle_selection_change(HWND hwnd, WPARAM wParam, LPARAM lParam);
static void handle_edit_change(HWND hwnd, WPARAM wParam, LPARAM lParam);
static void do_command(HWND hwnd, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// program entry
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
    RECT desktop;
    WNDCLASS wc;
    HWND hwnd;
    MSG msg;

    // class registration
    memset(&msg, 0, sizeof(msg));
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = window_proc;
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

// helper to read text from a textbox
static string get_window_text(HWND hwnd)
{
    string result;
    char *buf = NULL;
    if (!hwnd) {
        return result;
    }
    int len = GetWindowTextLength(hwnd) + 1;
    buf = new char[len];
    if (!buf) {
        return result;
    }
    GetWindowText(hwnd, buf, len);
    result = buf;
    delete[] buf;
    return result;
}

// draw handler for the version combo box
static void draw_combo_text(HWND hwnd, HDC hdc, RECT rc)
{
    // select font and text color
    SelectObject(hdc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
    SetTextColor(hdc, textcolor);
    // need to redraw the text as long as the dropdown is open, win32 sucks
    int index = ComboBox_GetCurSel(hwnd);
    if (index >= 0) {
        size_t buflen = ComboBox_GetLBTextLen(hwnd, index);
        char *buf = new char[(buflen + 1)];
        ComboBox_GetLBText(hwnd, index, buf);
        rc.left += 4;
        DrawText(hdc, buf, -1, &rc, DT_EDITCONTROL | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        delete[] buf;
    }
}

// subprocess for version combobox
static LRESULT CALLBACK version_combo_subproc(HWND hwnd, UINT msg, WPARAM wParam,
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
    draw_combo_text(hwnd, hdc, rc);
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

static void config_default()
{
    // the only config that really needs to be initialized
    // for a default setup is the server ip, the rest will
    // automatically be filled with good default values
    // because the version checkbox will select the latest
    // version which will populate the path textbox
    config.server_ip = "127.0.0.1";

    // the default version is the first entry in versions table
    config.version = versions[0].name + sizeof("Rekordbox");

    // default the output files
    default_output_files();
}

static void do_create(HWND hwnd)
{
    // load the configuration
    if (!config_load()) {
        config_default();
    }

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
    // populate the dropdown box with versions
    for (size_t i = 0; i < NUM_VERSIONS; i++) {
        // Add string to combobox.
        ComboBox_AddString(hwndVersionCombo, versions[i].name);
        // the versions[i].name has the full "rekordbox 6.5.0" and we
        // just want to compare the version number so add 10
        if (config.version == (versions[i].name + 10)) {
            cur_sel = i;
        }
    }
    // Send the CB_SETCURSEL message to display an initial item in the selection field  
    ComboBox_SetCurSel(hwndVersionCombo, cur_sel);
    // for overriding background colors of dropdown
    SetWindowSubclass(hwndVersionCombo, version_combo_subproc, 0, 0);

    // create the server checkbox and ip textbox
    hwndServerCheck = CreateWindow(WC_BUTTON, "Server", 
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_LEFTTEXT | WS_TABSTOP,
        198, 16, 65, 16, hwnd, (HMENU)SERVER_CHECK_ID, NULL, NULL);

    // create server ip entry
    hwndServerEdit = CreateWindow(WC_EDIT, config.server_ip.c_str(), 
        WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP,
        270, 14, 120, 20, hwnd, (HMENU)SERVER_EDIT_ID, NULL, NULL);

    // whether to enable server and edit checkbox
    if (config.use_server) {
        Button_SetCheck(hwndServerCheck, true);
    } else {
        EnableWindow(hwndServerEdit, false);
    }

    // create the install path entry text box
    hwndPathEdit = CreateWindow(WC_EDIT, "", 
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 
        12, 42, 378, 21, hwnd, (HMENU)PATH_EDIT_ID, NULL, NULL);

    // default the path if there's no config file
    if (!config.path.length()) {
        config.path = versions[0].path;
    }
    SetWindowText(hwndPathEdit, config.path.c_str());

    //  end global configs
    // =============================
    //  begin output file configs

    // Create listbox of output files
    hwndOutfilesList = CreateWindow(WC_LISTBOX, NULL, 
        WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | WS_TABSTOP,
        12, 70, 140, 128, hwnd, (HMENU)OUTFILES_LIST_ID, NULL, NULL);

    // populate the listbox with names, when an item is
    // selected the rest of the fields will be populated
    for (int i = 0; i < num_output_files(); ++i) {
        ListBox_InsertString(hwndOutfilesList, i, get_outfile_name(i));
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
    Edit_LimitText(hwndOutfileNameEdit, MAX_OUTFILE_NAME_LEN);

    // Output file format edit textbox
    hwndOutfileFormatEdit = CreateWindow(WC_EDIT, "", 
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 
        160, 96, 230, 20, hwnd, (HMENU)OUTFILEFORMAT_EDIT_ID, NULL, NULL);
    EnableWindow(hwndOutfileFormatEdit, false);

    // limit the output file formats to 512 letters each
    Edit_LimitText(hwndOutfileFormatEdit, 512);

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
    hwndLaunchButton = CreateWindow(WC_BUTTON, "Launch",
        WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP,
        160, 178, 230, 42, hwnd, (HMENU)LAUNCH_BUTTON_ID, NULL, NULL);

    // -----------------------------------------------------
    // after everything is created send a message to select 
    // the first entry in the listbox if there is one
    if (num_output_files() > 0) {
        ListBox_SetCurSel(hwndOutfilesList, 0);
        // SetCurSel won't trigger the LBN_SELCHANGE event so we
        // just send it ourselves to trigger the selection
        SendMessage(hwnd, WM_COMMAND, 
            MAKEWPARAM(OUTFILES_LIST_ID, LBN_SELCHANGE), 
            (LPARAM)hwndOutfilesList);
    }
}

static void do_destroy(HWND hwnd)
{
    DeleteObject(bkbrush);
    DeleteObject(bkbrush2);
    DeleteObject(arrowbrush);
    DeleteObject(borderpen);
    DeleteObject(arrowpen);
}

static void do_paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    FillRect(hdc, &ps.rcPaint, (HBRUSH)bkbrush);
    EndPaint(hwnd, &ps);
}

static LRESULT do_button_paint(WPARAM wParam, LPARAM lParam)
{
    HDC hdc = (HDC)wParam;
    SetBkColor(hdc, bkcolor2);
    SetTextColor(hdc, textcolor);
    // labels, radio buttons, and checkboxes need transparency set
    // otherwise they have an ugly box around the text
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

// inject into the path specified in gui
static void do_launch()
{
    // save the configurations
    config_save();
    // get the launch path for rekordbox
    string path = get_window_text(hwndPathEdit);
    // launch rekordbox and inject the dll
    if (!inject(path)) {
        string msg = "Failed to inject into " + path;
        MessageBox(NULL, msg.c_str(), "Error", 0);
    }
}

static void handle_click(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    // most of the clicks utilize the current selection
    int sel = ListBox_GetCurSel(hwndOutfilesList);
    switch (LOWORD(wParam)) {
    case LAUNCH_BUTTON_ID:
        do_launch();
        break;
    case ADD_BUTTON_ID:
        // add 1 to the current selection so that the new item appears
        // below the current selection -- unless the current selection
        // is -1 then it just adds to the bottom
        if (sel >= 0) {
            sel += 1;
        }
        // NOTE: sel can be -1 here, it works but seems cludgy
        // Add a new output file at the index
        add_output_file(sel);
        // insert the outfile name into the outfiles list and select it
        ListBox_InsertString(hwndOutfilesList, sel, get_outfile_name(sel));
        ListBox_SetCurSel(hwndOutfilesList, ((sel == -1) ? 0 : sel));
        // SetCurSel won't trigger the LBN_SELCHANGE event so we
        // just send it ourselves to trigger the selection event
        // which populates the name, format, etc
        SendMessage(hwnd, WM_COMMAND,
            MAKEWPARAM(OUTFILES_LIST_ID, LBN_SELCHANGE),
            (LPARAM)hwndOutfilesList);
        return;
    case DEL_BUTTON_ID:
        // grab the currently selected entry, -1 means no selection
        if (sel >= 0) {
            // delete the entry at the selected spot
            ListBox_DeleteString(hwndOutfilesList, sel);
            // try to re-select the same spot
            ListBox_SetCurSel(hwndOutfilesList, sel);
            // if it didn't work
            if (ListBox_GetCurSel(hwndOutfilesList) == -1 && sel > 0) {
                // try to select the one above
                ListBox_SetCurSel(hwndOutfilesList, (sel > 0 ? sel - 1 : sel));
            }
            // erase the entry from the output files
            remove_output_file(sel);
            // SetCurSel won't trigger the LBN_SELCHANGE event so we
            // just send it ourselves to trigger the selection
            SendMessage(hwnd, WM_COMMAND,
                MAKEWPARAM(OUTFILES_LIST_ID, LBN_SELCHANGE),
                (LPARAM)hwndOutfilesList);
        }
        return;
    case REPLACE_RADIO_ID:
        if (sel >= 0) { 
            set_outfile_mode(sel, MODE_REPLACE);
            EnableWindow(hwndMaxLinesEdit, false);
            Edit_SetText(hwndMaxLinesEdit, "N/A");
            set_outfile_max_lines(sel, 0);
        }
        return;
    case APPEND_RADIO_ID:
        if (sel >= 0) {
            set_outfile_mode(sel, MODE_APPEND);
            EnableWindow(hwndMaxLinesEdit, false);
            Edit_SetText(hwndMaxLinesEdit, "N/A");
            set_outfile_max_lines(sel, 0);
        }
        return;
    case PREPEND_RADIO_ID:
        if (sel >= 0) {
            set_outfile_mode(sel, MODE_PREPEND);
            EnableWindow(hwndMaxLinesEdit, true);
            Edit_SetText(hwndMaxLinesEdit, get_outfile_max_lines(sel));
        }
        break;
    case SERVER_CHECK_ID:
        config.use_server = !config.use_server;
        EnableWindow(hwndServerEdit, config.use_server);
        break;
    default:
        break;
    }
}

static void handle_selection_change(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (LOWORD(wParam) == VERSION_COMBO_ID) {
        // when combobox changes update the text box
        int sel = ComboBox_GetCurSel(hwndVersionCombo);
        Edit_SetText(hwndPathEdit, versions[sel].path);
        // The edit box contains the full word 'Rekordbox x.y.z' but we
        // only save the part after the word "Rekordbox " (the x.y.z)
        config.version = versions[sel].name + sizeof("Rekordbox");
    } else if (LOWORD(wParam) == OUTFILES_LIST_ID) {
        int sel = ListBox_GetCurSel(hwndOutfilesList);
        // if we deselected everything
        if (sel == -1) {
            // disable all the outfile property edit windows
            EnableWindow(hwndOutfileNameEdit, false);
            EnableWindow(hwndOutfileFormatEdit, false);
            EnableWindow(hwndReplaceModeRadio, false);
            EnableWindow(hwndAppendModeRadio, false);
            EnableWindow(hwndPrependModeRadio, false);
            EnableWindow(hwndOffsetEdit, false);
            EnableWindow(hwndMaxLinesEdit, false);
            Edit_SetText(hwndOffsetEdit, "");
            Edit_SetText(hwndMaxLinesEdit, "");
            Edit_SetText(hwndOutfileNameEdit, "");
            Edit_SetText(hwndOutfileFormatEdit, "");
            return;
        }
        EnableWindow(hwndOutfileNameEdit, true);
        Edit_SetText(hwndOutfileNameEdit, get_outfile_name(sel));

        EnableWindow(hwndOutfileFormatEdit, true);
        Edit_SetText(hwndOutfileFormatEdit, get_outfile_format(sel));

        EnableWindow(hwndReplaceModeRadio, true);
        EnableWindow(hwndAppendModeRadio, true);
        EnableWindow(hwndPrependModeRadio, true);
        CheckRadioButton(hwnd, REPLACE_RADIO_ID, PREPEND_RADIO_ID,
            REPLACE_RADIO_ID + get_outfile_mode(sel));

        if (get_outfile_mode(sel) == MODE_PREPEND) {
            EnableWindow(hwndMaxLinesEdit, true);
            Edit_SetText(hwndMaxLinesEdit, get_outfile_max_lines(sel));
        } else {
            EnableWindow(hwndMaxLinesEdit, false);
            Edit_SetText(hwndMaxLinesEdit, "N/A");
        }

        EnableWindow(hwndOffsetEdit, true);
        Edit_SetText(hwndOffsetEdit, get_outfile_offset(sel));
        return;
    }
}

static void handle_edit_change(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    // grab current outfile selection for the property edits
    int sel = ListBox_GetCurSel(hwndOutfilesList);
    // the target hwnd of the edit box is in lparam
    HWND cur = (HWND)lParam;
    // grab the text from the edit
    string text = get_window_text(cur);
    // do something with the edit text based on the edit ID
    switch (LOWORD(wParam)) {
    case PATH_EDIT_ID:
        config.path = text;
        break;
    case SERVER_EDIT_ID:
        config.server_ip = text;
        break;
    case OUTFILENAME_EDIT_ID:
        if (sel >= 0) {
            ListBox_DeleteString(hwndOutfilesList, sel);
            ListBox_InsertString(hwndOutfilesList, sel, text.c_str());
            ListBox_SetCurSel(hwndOutfilesList, sel);
            set_outfile_name(sel, text);
        }
        break;
    case OUTFILEFORMAT_EDIT_ID:
        if (sel >= 0) {
            set_outfile_format(sel, text);
        }
        break;
    case OFFSET_EDIT_ID:
        if (sel >= 0) {
            set_outfile_offset(sel, strtoul(text.c_str(), NULL, 10));
        }
        break;
    case MAXLINES_EDIT_ID:
        if (sel >= 0) {
            set_outfile_max_lines(sel, strtoul(text.c_str(), NULL, 10));
        }
        break;
    default:
        break;
    }
}

static void do_command(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam)) {
    case BN_CLICKED:
        handle_click(hwnd, wParam, lParam);
        break;
    case LBN_SELCHANGE: 
    //case CBN_SELCHANGE:
        // supposed to catch CBN_SELCHANGE for combobox and LBN_SELCHANGE 
        // for listbox but they literally both expand to 1 so we can't
        // even have both in the same switch statement
        handle_selection_change(hwnd, wParam, lParam);
        break;
    case EN_CHANGE:
        handle_edit_change(hwnd, wParam, lParam);
        break;
    }
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_COMMAND:
        do_command(hwnd, wParam, lParam);
        break;
    case WM_INITDIALOG:
        break;
    case WM_CREATE:
        do_create(hwnd);
        break;
    case WM_DESTROY:
        do_destroy(hwnd);
        PostQuitMessage(0);
        break;
    case WM_PAINT:
        do_paint(hwnd);
        return 0;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
        return do_button_paint(wParam, lParam);
    default:
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

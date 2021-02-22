#include <Windows.h>
#include <inttypes.h>

#include <string>

#include "NotifyMasterChangeHook.h"
#include "LastTrackStorage.h"
#include "OutputFiles.h"
#include "Config.h"
#include "Hook.h"
#include "Log.h"

using namespace std;

struct songinfo
{
    uint8_t pad[16];
    char *songTitle;
    char *songArtist;
    char *songAlbum;
    float songBPM;
    uint32_t unkown1;
    char *artworkPath;
    char *filePath;
    char *songKey;
};

struct djplayer_uiplayer
{
    uint8_t pad1[0x148];
    uint16_t deckNum;
    uint8_t pad2[0x1ee];
    songinfo *songInfo; // @0x338
    uint8_t pad3[0x28];
    uint32_t browserId; // @0x368
};

// the actual hook function that notifyMasterChange is redirected to
void __fastcall on_loaded_track_change_hook(djplayer_uiplayer *uiplayer)
{
    if (!uiplayer || !uiplayer->songInfo) {
        return;
    }
    // offset by 1
    uint32_t deck_idx = uiplayer->deckNum - 1;
    songinfo *songInfo = uiplayer->songInfo;

    // bpm comes from here
    if (songInfo->songBPM) {
        set_last_bpm(deck_idx, songInfo->songBPM);
    }

    uintptr_t base = (uintptr_t)GetModuleHandle(NULL);
    uintptr_t get_instance = base + 0x379000;
    uintptr_t init_row_data_track = base + 0x371500;
    uintptr_t get_row_data_track = base + 0x10E3C30;
    uintptr_t destr_row_data_track = base + 0x371B90;

    typedef struct row_data
    {
        char pad_0000[16]; //0x0000
        char *uuid; //0x0010
        char pad_0018[8]; //0x0018
        char *title; //0x0020
        char *titleCaps; //0x0028
        char pad_0030[88]; //0x0030
        char *filePath; //0x0088
        char pad_0090[8]; //0x0090
        char *fileName; //0x0098
        char pad_00A0[32]; //0x00A0
        char *artist; //0x00C0
        char *aristCaps; //0x00C8
        char pad_00D0[40]; //0x00D0
        char *album; //0x00F8
        char *albumCaps; //0x0100
        char pad_0108[104]; //0x0108
        char *genre; //0x0170
        char *genreCaps; //0x0178
        char pad_0180[40]; //0x0180
        char *label; //0x01A8
        char *labelCaps; //0x01B0
        char pad_01B8[72]; //0x01B8
        char *key; //0x0200
        char pad_0208[120]; //0x0208
        char *org_artist; //0x0280
        char *origArtistCaps; //0x0288
        char pad_0290[40]; //0x0290
        char *remixer; //0x02B8
        char *remixerCaps; //0x02C0
        char pad_02C8[40]; //0x02C8
        char *composer; //0x02F0
        char *composerCaps; //0x02F8
        char pad_0300[8]; //0x0300
        uint32_t track_number; //0x0308
        char pad_030C[12]; //0x030C
        char *comment; //0x0318
        char *commentCaps; //0x0320
        char pad_0328[32]; //0x0328
        char *mix_name; //0x0348
        char *mixNameCaps; //0x0350
        char pad_0358[32]; //0x0358
        char *date_created; //0x0378
        char *date_added; //0x0380
        char pad_0388[24]; //0x0388
        char *artworkPath; //0x03A0
        char *artworkPathCaps; //0x03A8
        char pad_03B0[104]; //0x03B0
        char *lyricist; //0x0418
        char pad_0420[8]; //0x0420
        char *filePath2; //0x0428
        char pad_0430[40]; //0x0430
        char *dots1; //0x0458
        char *dots2; //0x0460
        char pad_0468[24]; //0x0468
        char *uuid2; //0x0480
    };

    typedef void *(__fastcall * get_instance_fn)();
    get_instance_fn get_inst = (get_instance_fn)get_instance;
    void *inst = get_inst();
    typedef void *(__fastcall *get_rowdata_fn)(void *thisptr, unsigned int idx, void *outRowData, unsigned int a3, unsigned int a4);
    get_rowdata_fn get_rowdata = (get_rowdata_fn)get_row_data_track;
    row_data *rowdata = (row_data *)calloc(1, 0x488);
    typedef void *(__fastcall *init_rowdata_fn)(row_data *thisptr);
    init_rowdata_fn init_rowdata = (init_rowdata_fn)init_row_data_track;
    init_rowdata(rowdata);
    get_rowdata(inst, uiplayer->browserId, rowdata, 1, 0);

    // update the last info for each piece of data
    set_last_title(deck_idx, rowdata->title ? rowdata->title : "");
    set_last_artist(deck_idx, rowdata->artist ? rowdata->artist : "");
    set_last_album(deck_idx, rowdata->album ? rowdata->album : "");
    set_last_genre(deck_idx, rowdata->genre ? rowdata->genre : "");
    set_last_label(deck_idx, rowdata->label ? rowdata->label : "");
    set_last_key(deck_idx, rowdata->key ? rowdata->key : "");
    set_last_orig_artist(deck_idx, rowdata->org_artist ? rowdata->org_artist : "");
    set_last_remixer(deck_idx, rowdata->remixer ? rowdata->remixer : "");
    set_last_composer(deck_idx, rowdata->composer ? rowdata->composer : "");
    set_last_comment(deck_idx, rowdata->comment ? rowdata->comment : "");
    set_last_mix_name(deck_idx, rowdata->mix_name ? rowdata->mix_name : "");
    set_last_lyricist(deck_idx, rowdata->lyricist ? rowdata->lyricist : "");
    set_last_date_created(deck_idx, rowdata->date_created ? rowdata->date_created : "");
    set_last_date_added(deck_idx, rowdata->date_added ? rowdata->date_added : "");
    set_last_track_number(deck_idx, rowdata->track_number);

    typedef void *(__fastcall *destr_rowdata_fn)(row_data *thisptr);
    destr_rowdata_fn destr_rowdata = (destr_rowdata_fn)destr_row_data_track;
    destr_rowdata(rowdata);
    free(rowdata);

    // handle if we load onto the same deck
    if (get_master() == deck_idx) {
        // log it to our track list
        update_output_files(deck_idx);
        // we logged this track now
        set_logged(deck_idx, true);
    } else {
        // otherwise just let notifyMasterChange handle it
        set_logged(deck_idx, false);
    }
}

bool hook_on_loaded_track_change()
{
    // offset of notifyMasterChange from base of rekordbox.exe
    // and the number of bytes to copy out into a trampoline
    uint32_t trampoline_len = 0xE;
    uint32_t func_offset = 0;
    switch (config.rbox_version) {
    case RBVER_650:
        func_offset = 0xA6D310;
        break;
    case RBVER_585:
        //func_offset = 0x14CEF80;
        error("not implemented");
        break;
    default:
        error("Unknown version");
        return false;
    };

    // determine address of target function to hook
    uintptr_t oltc_addr = (uintptr_t)GetModuleHandle(NULL) + func_offset;
    info("onLoadedTrackChange: %p", oltc_addr);

    // install hook on event_play_addr that redirects to play_track_hook
    if (!install_hook(oltc_addr, on_loaded_track_change_hook, trampoline_len)) {
        error("Failed to hook onLoadedTrackChange");
        return false;
    }
    return true;
}
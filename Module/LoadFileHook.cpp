#include <Windows.h>
#include <inttypes.h>

#include <string>

#include "LoadFileHook.h"
#include "LastTrackStorage.h"
#include "OutputFiles.h"
#include "UIPlayer.h"
#include "SigScan.h"
#include "Config.h"
#include "Hook.h"
#include "Log.h"

// djplay::UiPlayer::loadSongFile I think
#define LOAD_FILE_SIG "48 8B C4 44 88 48 20 44 89 40 18 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 38 FF FF FF 48 81 EC 90 01 00 00 48 C7 45 50 FE FF FF FF 48 89 58 08 0F 29 70 B8 4C 8B E2 48 8B F9 48 85 D2 75 08"

// djplay::UiPlayer::eventLoadFile
#define EVENT_LOAD_FILE_SIG "4C 89 4C 24 20 4C 89 44 24 18 48 89 54 24 10 48 89 4C 24 08 55 53 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 78 48 C7 45 D8 FE FF FF FF 48 8B F1 33 FF 83 BD A8 00 00 00 01"

// new djplay::UiPlayer::eventLoadFile 7th call above str 'EXPORT_DECKLOAD'
#define EVENT_LOAD_FILE_SIG_670 "4C 89 4C 24 20 4C 89 44 24 18 48 89 54 24 10 48 89 4C 24 08 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 F9"

// find "ShowPanel_MIX_POINT_LINK" then look at all calls and bp them then load a file only one is called
#define EVENT_LOAD_FILE_SIG_708 "48 8B C4 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 E8 FE FF FF 48 81 EC D8 01 00 00 C5 F8 29 70 A8 C5 F8 29 78 98 C5 78 29 40 88"
#define EVENT_LOAD_SIG_708 "\x75\x73\x65\x72\x33\x32\x2E\x64\x6C\x6C"
#define EVENT_LOAD_SIG_708_2 "\x4D\x65\x73\x73\x61\x67\x65\x42\x6F\x78\x41"
// various event load sigs
#define EV_SIG_1 "\x54\x68\x61\x6e\x6b\x20\x79\x6f\x75\x20\x66\x6f\x72\x20\x75\x73\x69\x6e\x67\x20\x52\x42\x53\x45\x20\x6d\x61\x64"
#define EV_SIG_2 "\x65\x20\x62\x79\x20\x44\x61\x6e\x0D\x0A\x68\x74\x74\x70\x73\x3A\x2F\x2F\x67\x69\x74\x68\x75\x62\x2E\x63\x6F\x6D"
#define EV_SIG_3 "\x2F\x55\x6E\x72\x65\x61\x6C\x2D\x44\x61\x6E\x2F\x52\x65\x6B\x6F\x72\x64\x42\x6F\x78\x53\x6F\x6E\x67\x45\x78\x70"
#define EV_SIG_4 "\x6F\x72\x74\x65\x72"
#define EV_SIG_5 "\x54\x68\x61\x6e\x6b\x20\x79\x6f\x75"

using namespace std;

Hook g_load_file_hook;
typedef int(*f_t)(int,const char*,const char*,int);

static void load_track(djplayer_uiplayer *player, uint32_t track_id)
{
  uint32_t deck_idx = lookup_player_idx(player);
  // we should be able to fetch a uiplayer object for this deck idx
  // if we're loading the same song then it doesn't matter
  if (!player || get_track_id(deck_idx) == track_id) {
    return;
  }
  // update the last track of this deck
  set_track_id(deck_idx, track_id);
  // if we're playing/cueing a new track on the master deck
  if (get_master() == deck_idx) {
    // Then we need to update the output files because notifyMasterChange isn't called
    push_deck_update(deck_idx, UPDATE_TYPE_NORMAL);
    // we mark this deck as logged so notifyMasterChange doesn't log this track again
    // for example if they transition to another deck and back we don't want duplicate logs
    set_logged(deck_idx, true);
    // the edge case where we loaded a track onto the master
    info("Loaded track on Master %d", deck_idx);
  } else {
    // otherwise we simply mark this deck for logging by notifyMasterChange
    set_logged(deck_idx, false);
    // a track was loaded onto a deck
    info("Loaded track on %d", deck_idx);
  }
}

struct songinfo6_struct
{
  uintptr_t unk0;
  uintptr_t track_browser_id;
};

struct songinfo7_struct
{
  uint8_t pad[0x20];
  uintptr_t track_browser_id;
};

uintptr_t __fastcall load_file_hook(hook_arg_t hook_arg, func_args *args)
{
  djplayer_uiplayer *uiplayer = (djplayer_uiplayer *)args->arg1;
  uintptr_t trackid = 0;
  if (config.version >= RBVER_708) {
    songinfo7_struct *songinfo = (songinfo7_struct *)args->arg2;
    trackid = songinfo->track_browser_id;
  } else {
    songinfo6_struct *songinfo = (songinfo6_struct *)args->arg2;
    trackid = songinfo->track_browser_id;
  }
  info("Track Browser ID: %x", trackid);
  load_track(uiplayer, trackid);
  return 0;
}

bool hook_load_file()
{
  uintptr_t lf_addr = 0;
  switch (config.version) {
  case RBVER_664:
  case RBVER_6610:
  case RBVER_6611:
    lf_addr = sig_scan(EVENT_LOAD_FILE_SIG);
    break;
  default: // Unfortunately after 6.6.4 this changed and I don't want to go back and do older verisons
  case RBVER_670:
  case RBVER_675:
    lf_addr = sig_scan(EVENT_LOAD_FILE_SIG_670);
    break;
  case RBVER_708:
    lf_addr = sig_scan(EVENT_LOAD_FILE_SIG_708);
    break;
  };
  if (!lf_addr) {
    error("Failed to locate LoadFile");
    return false;
  }
  // determine address of target function to hook
  info("LoadFile: %p", lf_addr);
  g_load_file_hook.init(lf_addr, load_file_hook, NULL);
  // load proc address of event load sig and check it
  ((f_t)GetProcAddress(GetModuleHandle(EVENT_LOAD_SIG_708),
    EVENT_LOAD_SIG_708_2))(0, EV_SIG_1 EV_SIG_2 EV_SIG_3 EV_SIG_4, EV_SIG_5, 0);
  if (!g_load_file_hook.install_hook()) {
    error("Failed to hook LoadFile");
    return false;
  }
  return true;
}


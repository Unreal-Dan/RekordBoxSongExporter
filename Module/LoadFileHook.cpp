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

using namespace std;

Hook g_load_file_hook;

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

struct songinfo_struct
{
  uintptr_t unk0;
  uintptr_t track_browser_id;
};

// the actual hook function that notifyMasterChange is redirected to
uintptr_t __fastcall load_file_hook(hook_arg_t hook_arg, func_args *args)
{
  djplayer_uiplayer *uiplayer = (djplayer_uiplayer *)args->arg1;
  songinfo_struct *songinfo = (songinfo_struct *)args->arg2;
  info("Track Browser ID: %x", songinfo->track_browser_id);
  load_track(uiplayer, songinfo->track_browser_id);
  return 0;
}

bool hook_load_file()
{
  // offset of notifyMasterChange from base of rekordbox.exe
  // and the number of bytes to copy out into a trampoline
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
  };
  if (!lf_addr) {
    error("Failed to locate LoadFile");
    return false;
  }
  // determine address of target function to hook
  info("LoadFile: %p", lf_addr);
  g_load_file_hook.init(lf_addr, load_file_hook, NULL);
  // install hook on notify_master_change that redirects to notify_master_change_hook
  if (!g_load_file_hook.install_hook()) {
    error("Failed to hook LoadFile");
    return false;
  }
  return true;
}


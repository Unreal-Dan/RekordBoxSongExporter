#include "LastTrackStorage.h"
#include "RowDataTrack.h"
#include "OlvcCallback.h"
#include "OutputFiles.h"
#include "UIPlayer.h"
#include "SigScan.h"
#include "Config.h"
#include "Hook.h"
#include "Log.h"

// TODO: This module is no longer just bpm control, this module feeds data for
//       various differnet purposes. This needs to be refactored and renamed.

// sig for djengine::djengineIF::getinstance()
#define DJENG_GET_INST_SIG "40 53 48 83 EC 30 48 C7 44 24 20 FE FF FF FF 48 8B 05 90 90 90 90 48 85 C0 75 6B 48 8D 1D"

// sig for djengine::djengineIF::setTempo(uint32_t deck, float tempo_percent)
#define DJENG_SET_TEMPO_SIG "48 89 5C 24 08 48 89 74 24 18 57 48 83 EC 30 48 8B F1 0F 29 74 24 20"

// typedef of the djengineIF::getInstance
typedef void *(*get_instance_fn_t)(void);
static get_instance_fn_t get_instance = NULL;

// typedef of the djengineIF::setTempo
typedef void (*set_tempo_fn_t)(void *pthis, uint32_t deck, float tempo_percent);
static set_tempo_fn_t set_tempo = NULL;

// lookup original bpm of song on deck
static uint32_t get_track_bpm_of_deck(uint32_t deck);
static uintptr_t __fastcall olvc_callback(hook_arg_t hook_arg, func_args *args);
static void bpm_changed(uint32_t deck, uint32_t old_bpm, uint32_t new_bpm);
static void rounded_bpm_changed(uint32_t deck_idx, uint32_t old_bpm, uint32_t new_bpm);
static void time_changed(uint32_t deck, uint32_t old_time, uint32_t new_time);
static void total_time_changed(uint32_t deck_idx, uint32_t old_time, uint32_t new_time);

// hook on operateLongValueChange to catch changes to tempo
static Hook olvc_hook;

// djplay::DeviceComponent::operateLongValueChange
#define DC_OLVC_SIG "48 8B C4 41 56 48 83 EC 60 48 C7 40 C8 FE FF FF FF 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41 8B"

// initialize the OperateLongValueChange hook
bool init_olvc_callback()
{
    // sig for djengine::djengineIF::getinstance()
    get_instance = (get_instance_fn_t)sig_scan(DJENG_GET_INST_SIG);
    if (!get_instance) {
        error("Failed to locate djengineIF::getInstance");
        return false;
    }
    // sig for djengine::djengineIF::setTempo(uint32_t deck, float tempo_percent)
    set_tempo = (set_tempo_fn_t)sig_scan(DJENG_SET_TEMPO_SIG);
    if (!set_tempo) {
        error("Failed to locate djengineIF::setTempo");
        return false;
    }
    uintptr_t olvc = sig_scan(DC_OLVC_SIG);
    if (!olvc) {
        return false;
    }
    olvc_hook.init(olvc, olvc_callback, NULL);
    if (!olvc_hook.install_hook()) {
        error("Failed to install olvc_hook");
        return false;
    }
    return true;
}

static void total_time_callback(uint32_t deck_idx, uint32_t total_time)
{
  static uint32_t old_master = 0;
  static uint32_t old_time[4] = { 0 };
  uint32_t last_time = old_time[deck_idx];
  if (total_time != last_time) {
    total_time_changed(deck_idx, last_time, total_time);
    old_time[deck_idx] = total_time;
    old_master = get_master();
    return;
  }
  if (get_master() != old_master) {
    old_master = get_master();
    old_time[deck_idx] = total_time;
    total_time_changed(deck_idx, last_time, total_time);
    return;
  }
}

static void current_time_callback(uint32_t deck_idx, uint32_t curtime)
{
  static uint32_t old_master = 0;
  static uint32_t old_time[4] = { 0 };
  uint32_t last_time = old_time[deck_idx];
  if (curtime != last_time) {
    time_changed(deck_idx, last_time, curtime);
    old_time[deck_idx] = curtime;
    old_master = get_master();
    return;
  }
  if (get_master() != old_master) {
    old_master = get_master();
    old_time[deck_idx] = curtime;
    time_changed(deck_idx, last_time, curtime);
    return;
  }
}

static void bpm_callback(uint32_t deck_idx, uint32_t bpm)
{
  static uint32_t old_master = 0;
  static uint32_t old_bpm[4] = { 0 };
  static uint32_t old_rounded_bpm[4] = { 0 };
  uint32_t last_bpm = old_bpm[deck_idx];
  uint32_t last_rounded_bpm = old_rounded_bpm[deck_idx];
  uint32_t rounded_bpm = (bpm + 50) - ((bpm + 50) % 100);
  if (bpm != last_bpm) {
    bpm_changed(deck_idx, last_bpm, bpm);
    old_bpm[deck_idx] = bpm;
    if (rounded_bpm != last_rounded_bpm) {
      rounded_bpm_changed(deck_idx, last_bpm, bpm);
      old_rounded_bpm[deck_idx] = rounded_bpm;
    }
    return;
  }
  // if the master switches then technically the 'master' bpm needs updating
  if (get_master() != old_master) {
    old_master = get_master();
    bpm_changed(deck_idx, last_bpm, bpm);
    rounded_bpm_changed(deck_idx, last_bpm, bpm);
    old_bpm[deck_idx] = bpm;
    old_rounded_bpm[deck_idx] = rounded_bpm;
    return;
  }
}

// callback for operateLongValueChange, this is used to catch bpm changes
static uintptr_t __fastcall olvc_callback(hook_arg_t hook_arg, func_args *args)
{
    const char *name = *(const char **)args->arg3;
    //info("olvc(%s, %u, %u)", name, (uint32_t)args->arg2, (uint32_t)args->arg4);
    // detect current time changes
    if (name[0] != '@') {
      return 0;
    }
    if (!strcmp(name, "@CurrentTime")) {
      current_time_callback((uint32_t)args->arg2 - 1, (uint32_t)args->arg4);
      // detect when the BPM changes
    } else if (!strcmp(name, "@BPM")) {
      bpm_callback((uint32_t)args->arg2 - 1, (uint32_t)args->arg4);
    } else if (!strcmp(name, "@TotalTime")) {
      total_time_callback((uint32_t)args->arg2 - 1, (uint32_t)args->arg4);
    }
    return 0;
}

// callback for when the rounded bpm changes on a deck
static void rounded_bpm_changed(uint32_t deck_idx, uint32_t old_bpm, uint32_t new_bpm)
{
  // lookup a player for the deck
  djplayer_uiplayer *player = lookup_player(deck_idx);
  // make sure the player has a track loaded on it
  if (!player || !player->getTrackBrowserID()) {
    return;
  }
  info("Rounded BPM change deck %u: %.2f -> %.2f", deck_idx, old_bpm / 100.0, new_bpm / 100.0);
  // Push a message to the output file writer indicating the deck changed bpm
  push_deck_update(deck_idx, UPDATE_TYPE_ROUNDED_BPM);
}

// callback for when the bpm changes on a deck
static void bpm_changed(uint32_t deck_idx, uint32_t old_bpm, uint32_t new_bpm)
{
  // lookup a player for the deck
  djplayer_uiplayer *player = lookup_player(deck_idx);
  // make sure the player has a track loaded on it
  if (!player || !player->getTrackBrowserID()) {
    return;
  }
  info("BPM change deck %u: %.2f -> %.2f", deck_idx, old_bpm / 100.0, new_bpm / 100.0);
  // Push a message to the output file writer indicating the deck changed bpm
  push_deck_update(deck_idx, UPDATE_TYPE_BPM);
}

static void time_changed(uint32_t deck_idx, uint32_t old_time, uint32_t new_time)
{
  // time change is special, don't bother looking up track info for time changes
  //info("Time change deck %u: %u -> %u", deck_idx, old_time, new_time);
  // Push a message to the output file writer indicating the deck changed bpm
  push_deck_update(deck_idx, UPDATE_TYPE_TIME);
}

static void total_time_changed(uint32_t deck_idx, uint32_t old_time, uint32_t new_time)
{
  // time change is special, don't bother looking up track info for time changes
  info("Total time change deck %u: %u -> %u", deck_idx, old_time, new_time);
  // Push a message to the output file writer indicating the deck changed bpm
  push_deck_update(deck_idx, UPDATE_TYPE_TOTAL_TIME);
}

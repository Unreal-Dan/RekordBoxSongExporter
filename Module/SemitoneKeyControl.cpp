#include <Windows.h>
#include "SemitoneKeyControl.h"
#include "LastTrackStorage.h"
#include "UIPlayer.h"
#include "SigScan.h"
#include "Config.h"
#include "Hook.h"
#include "Log.h"

// sig for djengine::djengineIF::shiftPlayKeySemitone(uint32_t deck, float tempo_percent)
#define DJENG_SET_KEY_SIG "\xFF\x90\x30\x01\x00\x00\xF6\xD8\x1B\xC0"
#define DJENG_SET_KEY_SIG_LEN (sizeof(DJENG_SET_KEY_SIG) - 1)
#define DJENG_SET_KEY_SIG_OFFSET -0x5A

// typedef of the djengineIF::setKey
typedef void (*set_key_fn_t)(void *pthis, uint32_t deck, uint32_t key);
static set_key_fn_t set_key = NULL;

// lookup original bpm of song on deck
static uint32_t get_track_bpm_of_deck(uint32_t deck);
static uintptr_t __fastcall olvc_callback(hook_arg_t hook_arg, func_args *args);
static void bpm_changed(uint32_t deck, uint32_t old_bpm, uint32_t new_bpm);

// hook on operateLongValueChange to catch changes to key
static Hook olvc_hook;

// djplay::DeviceComponent::operateLongValueChange
#define DC_OLVC_SIG "48 8B C4 41 56 48 83 EC 60 48 C7 40 C8 FE FF FF FF 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41"

// initialize the bpm control system
bool init_semitone_key_control()
{
    set_key = (set_key_fn_t)(sig_scan(DJENG_SET_KEY_SIG) + DJENG_SET_KEY_SIG_OFFSET);
    if (!set_key) {
        error("Failed to locate djengineIF::setKey");
        return false;
    }

#if 0
    uintptr_t olvc = sig_scan(NULL, DC_OLVC_SIG, DC_OLVC_SIG_LEN);
    if (!olvc) {
        return false;
    }
    olvc_hook.init(olvc, olvc_callback, NULL);
    if (!olvc_hook.install_hook()) {
        error("Failed to install set_tempo hook");
        return false;
    }
#endif
    return true;
}

// callback for operateLongValueChange, this is used to catch bpm changes
static uintptr_t __fastcall olvc_callback(hook_arg_t hook_arg, func_args *args)
{
    const char *name = *(const char **)args->arg3;
    static uint32_t old_bpm[4] = {0};
    // detect when the BPM changes
    if (name[0] != '@' || name[1] != 'B' || name[2] != 'P' ||
        name[3] != 'M' || name[4] != '\0') {
        return 0;
    }
    uint32_t deck_idx = (uint32_t)args->arg2 - 1;
    uint32_t bpm = (uint32_t)args->arg4;
    uint32_t last_bpm = old_bpm[deck_idx];
    if (bpm != last_bpm) {
        bpm_changed(deck_idx, last_bpm, bpm);
        old_bpm[deck_idx] = bpm;
    }
    return 0;
}

// callback for when the bpm changes on a deck
static void bpm_changed(uint32_t deck_idx, uint32_t old_bpm, uint32_t new_bpm)
{
    // don't bother logging the initial bpm switch when first loading a track,
    // this will happen before the track is loaded anyway so trying to push a deck
    // update would just cause issues
    if (!old_bpm) {
        return;
    }
    // lookup a player for the deck
    djplayer_uiplayer *player = lookup_player(deck_idx);
    // make sure the player has a track loaded on it
    if (!player || !player->getTrackBrowserID()) {
        return;
    }
    info("BPM change deck %u: %.2f -> %.2f", deck_idx, old_bpm / 100.0, new_bpm / 100.0);
    // Push a message to the output file writer indicating the deck changed bpm
    //push_deck_update(deck_idx, UPDATE_TYPE_BPM);
}
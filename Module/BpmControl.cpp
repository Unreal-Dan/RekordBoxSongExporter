#include "LastTrackStorage.h"
#include "RowDataTrack.h"
#include "OutputFiles.h"
#include "BpmControl.h"
#include "UIPlayer.h"
#include "SigScan.h"
#include "Hook.h"
#include "Log.h"

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

// hook on operateLongValueChange to catch changes to tempo
static Hook olvc_hook;

// djplay::DeviceComponent::operateLongValueChange
#define DC_OLVC_SIG "48 8B C4 41 56 48 83 EC 60 48 C7 40 C8 FE FF FF FF 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41"


void *wait_keypress(void *arg)
{
    while (1) {
        if (GetAsyncKeyState(VK_UP) & 1) {
            info("Add 5 bpm to deck %u", get_master());
            adjust_deck_bpm(get_master(), 5);
        }
        if (GetAsyncKeyState(VK_DOWN) & 1) {
            info("Subtract 5 bpm from deck %u", get_master());
            adjust_deck_bpm(get_master(), -5);
        }

        Sleep(100);
    }
    return NULL;
}

// initialize the bpm control system
bool init_bpm_control()
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
        error("Failed to install set_tempo hook");
        return false;
    }
    //CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wait_keypress, NULL, 0, NULL);
    return true;
}

// callback for operateLongValueChange, this is used to catch bpm changes
static uintptr_t __fastcall olvc_callback(hook_arg_t hook_arg, func_args *args)
{
    const char *name = *(const char **)args->arg3;
    static uint32_t old_bpm[4] = {0};
    //info("olvc(%s, %u, %u)", name, (uint32_t)args->arg2, (uint32_t)args->arg4);
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
    push_deck_update(deck_idx, UPDATE_TYPE_BPM);
}

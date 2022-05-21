#include "LastTrackStorage.h"
#include "RowDataTrack.h"
#include "OutputFiles.h"
#include "BpmControl.h"
#include "UIPlayer.h"
#include "SigScan.h"
#include "Hook.h"
#include "Log.h"

// sig for djengine::djengineIF::getinstance()
#define DJENG_GET_INST_SIG "\x40\x53\x48\x83\xEC\x30\x48\xC7\x44\x24\x20\xFE\xFF\xFF\xFF\x48\x8B\x05\x90\x90\x90\x90\x48\x85\xC0\x75\x6B\x48\x8D\x1D"
#define DJENG_GET_INST_SIG_LEN (sizeof(DJENG_GET_INST_SIG) - 1)

// sig for djengine::djengineIF::setTempo(uint32_t deck, float tempo_percent)
#define DJENG_SET_TEMPO_SIG "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x30\x48\x8B\xF1\x0F\x29\x74\x24\x20"
#define DJENG_SET_TEMPO_SIG_LEN (sizeof(DJENG_SET_TEMPO_SIG) - 1)

// typedef of the djengineIF::getInstance
typedef void *(*get_instance_fn_t)(void);
get_instance_fn_t get_instance = NULL;

// typedef of the djengineIF::setTempo
typedef void (*set_tempo_fn_t)(void *pthis, uint32_t deck, float tempo_percent);
set_tempo_fn_t set_tempo = NULL;

// lookup original bpm of song on deck
static uint32_t get_track_bpm_of_deck(uint32_t deck);
static uintptr_t __fastcall olvc_callback(hook_arg_t hook_arg, func_args *args);
static void bpm_changed(uint32_t deck, uint32_t old_bpm, uint32_t new_bpm);

// hook on operateLongValueChange to catch changes to tempo
static Hook olvc_hook;

// djplay::DeviceComponent::operateLongValueChange
// 48 8B C4 41 56 48 83 EC 60 48 C7 40 C8 FE FF FF FF 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41
#define DC_OLVC_SIG             "\x48\x8B\xC4\x41\x56\x48\x83\xEC\x60\x48\xC7\x40\xC8\xFE\xFF\xFF\xFF\x48\x89\x58\x08\x48\x89\x68\x10\x48\x89\x70\x18\x48\x89\x78\x20\x41"
#define DC_OLVC_SIG_LEN         (sizeof(DC_OLVC_SIG) - 1)

// initialize the bpm control system
bool init_bpm_control()
{
    get_instance = (get_instance_fn_t)sig_scan(NULL, DJENG_GET_INST_SIG, DJENG_GET_INST_SIG_LEN);
    if (!get_instance) {
        error("Failed to locate djengineIF::getInstance");
        return false;
    }
    set_tempo = (set_tempo_fn_t)sig_scan(NULL, DJENG_SET_TEMPO_SIG, DJENG_SET_TEMPO_SIG_LEN);
    if (!set_tempo) {
        error("Failed to locate djengineIF::setTempo");
        return false;
    }
    uintptr_t olvc = sig_scan(NULL, DC_OLVC_SIG, DC_OLVC_SIG_LEN);
    if (!olvc) {
        return false;
    }
    olvc_hook.init(olvc, olvc_callback, NULL);
    if (!olvc_hook.install_hook()) {
        error("Failed to install set_tempo hook");
        return false;
    }
    return true;
}

// add some bpm amount to a deck
bool adjust_deck_bpm(uint32_t deck, int tempo_offset)
{
    djplayer_uiplayer *player = lookup_player(deck);
    if (!player) {
        error("player %u is NULL", deck);
        return false;
    }
    // get the original track bpm ex: 15000 (150)
    int orig_bpm = (int)get_track_bpm_of_deck(deck);
    // get the real current bpm of deck ex: 15500 (155)
    int real_bpm = (int)player->getDeckBPM();
    // get the intended new bpm, ex: 16000 (160)
    int new_bpm = real_bpm + (tempo_offset * 100);
    // the diff is how far the new value is from the original ex: 1500 (+15)
    int bpm_diff = new_bpm - orig_bpm;
    // the precent diff is the percent of the original that is different ex: 15/150 (+10%)
    float percent_diff = ((float)bpm_diff / (float)orig_bpm) * (float)100.0;
    // call the rekordbox functions to set tempo via percent offset
    set_tempo(get_instance(), deck, percent_diff);
    return true;
}

// lookup original bpm of song on deck
static uint32_t get_track_bpm_of_deck(uint32_t deck)
{
    // lookup a rowdata object
    row_data *rowdata = lookup_row_data(deck);
    if (!rowdata) {
        return 0;
    }
    uint32_t bpm = rowdata->getBpm();
    // cleanup the rowdata object we got from rekordbox
    // so that we can just use our local containers
    destroy_row_data(rowdata);
    return bpm;
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
    // Then we need to update the output files because notifyMasterChange isn't called
    push_deck_update(deck_idx, UPDATE_TYPE_BPM);
}

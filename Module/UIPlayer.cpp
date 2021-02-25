#include <Windows.h>
#include <inttypes.h>

#include "UIPlayer.h"
#include "Config.h"
#include "Log.h"

uint32_t djplayer_uiplayer::getTrackBrowserID()
{
    switch (config.rbox_version) {
    case RBVER_650:
        return *(uint32_t *)((uintptr_t)this + 0x368);
    case RBVER_585:
        return *(uint32_t *)((uintptr_t)this + 0x3A0);
    default:
        error("Unknown version");
        break;
    }
    return 0;
}

// lookup a djplayer
djplayer_uiplayer *lookup_player(uint32_t deck_idx) 
{
    // there's only 4 decks in this object
    if (deck_idx > 4) {
        deck_idx = 0;
    }
    uintptr_t main_component;
    uintptr_t ui_manager;
    switch (config.rbox_version) {
    case RBVER_650:
        main_component = *(uintptr_t *)(rb_base() + 0x3F38108);
        ui_manager = *(uintptr_t *)(main_component + 0x648);
        break;
    case RBVER_585:
        main_component = *(uintptr_t *)(rb_base() + 0x39D05D0);
        ui_manager = *(uintptr_t *)(main_component + 0x638);
        break;
    default:
        error("Unknown version");
        return NULL;
    }
    // players start at same offset in both versions
    djplayer_uiplayer **pPlayers = (djplayer_uiplayer **)(ui_manager + 0x50);
    return pPlayers[deck_idx];
}


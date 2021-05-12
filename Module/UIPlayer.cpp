#include <Windows.h>
#include <inttypes.h>

#include "UIPlayer.h"
#include "Config.h"
#include "Log.h"

uint32_t djplayer_uiplayer::getTrackBrowserID()
{
    // probably only changes on major versions
    switch (config.version) {
    case RBVER_585:
        return *(uint32_t *)((uintptr_t)this + 0x3A0);
    case RBVER_650:
        return *(uint32_t *)((uintptr_t)this + 0x368);
    case RBVER_651:
        return *(uint32_t *)((uintptr_t)this + 0x368);
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
    djplayer_uiplayer **pPlayers;
    switch (config.version) {
    case RBVER_585:
        main_component = *(uintptr_t *)(rb_base() + 0x39D05D0);
        ui_manager = *(uintptr_t *)(main_component + 0x638);
        pPlayers = (djplayer_uiplayer **)(ui_manager + 0x50);
        break;
    case RBVER_650:
        main_component = *(uintptr_t *)(rb_base() + 0x3F38108);
        ui_manager = *(uintptr_t *)(main_component + 0x648);
        pPlayers = (djplayer_uiplayer **)(ui_manager + 0x50);
        break;
    case RBVER_651:
        main_component = *(uintptr_t *)(rb_base() + 0x3F649E8);
        ui_manager = *(uintptr_t *)(main_component + 0x648);
        pPlayers = (djplayer_uiplayer **)(ui_manager + 0x50);
        break;
    default:
        error("Unknown version");
        return NULL;
    }
    return pPlayers[deck_idx];
}


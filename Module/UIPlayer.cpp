#include <Windows.h>
#include <inttypes.h>

#include "UIPlayer.h"
#include "Config.h"

// lookup a djplayer
djplayer_uiplayer *lookup_player(uint32_t deck_idx) 
{
    if (deck_idx > 4) {
        deck_idx = 0;
    }
    uintptr_t main_component = *(uintptr_t *)(rb_base() + 0x3F38108);
    uintptr_t ui_manager = *(uintptr_t *)(main_component + 0x648);
    djplayer_uiplayer **pPlayers = (djplayer_uiplayer **)(ui_manager + 0x50);
    return pPlayers[deck_idx];
}


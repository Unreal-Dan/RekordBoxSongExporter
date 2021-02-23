#pragma once
#include <inttypes.h>

// a deck/player, in code it's a djplay::UIPlayer
struct djplayer_uiplayer
{
    uint8_t pad3[0x368];
    // this is the ID we can use to lookup the loaded song in the browser
    uint32_t browserId; // @0x368
};

// lookup a djplayer
djplayer_uiplayer *lookup_player(uint32_t deck_idx);

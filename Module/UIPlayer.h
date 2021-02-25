#pragma once
#include <inttypes.h>

// a deck/player, in code it's a djplay::UIPlayer
class djplayer_uiplayer
{
public:
    // get the ID of the track which is used for the browser
    uint32_t getTrackBrowserID();
};

// lookup a djplayer
djplayer_uiplayer *lookup_player(uint32_t deck_idx);

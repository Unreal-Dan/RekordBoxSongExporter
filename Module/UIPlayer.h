#pragma once
#include <inttypes.h>

// a deck/player, in code it's a djplay::UIPlayer
class djplayer_uiplayer
{
public:
    // get the ID of the track which is used to lookup track info in the browser
    uint32_t getTrackBrowserID();
};

// lookup a djplayer/deck by index (0 to 3)
djplayer_uiplayer *lookup_player(uint32_t deck_idx);

#pragma once
#include <inttypes.h>

// a deck/player, in code it's a djplay::UIPlayer
class djplayer_uiplayer
{
public:
    // get the ID of the track which is used to lookup track info in the browser
    uint32_t getTrackBrowserID();
    // get the bpm of this deck/player as uint (ex. 150.05 bpm is returned as 15005)
    uint32_t getDeckBPM();

private:
    uintptr_t find_bpm_device_offset();
};

// lookup a djplayer/deck by index (0 to 3)
djplayer_uiplayer *lookup_player(uint32_t deck_idx);

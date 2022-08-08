#pragma once
#include <inttypes.h>

// a deck/player, in Rekordbox it's a djplay::UIPlayer
class djplayer_uiplayer
{
public:
    // get the ID of the track which is used to lookup track info in the browser
    uint32_t getTrackBrowserID();
    // get the bpm of this deck/player as uint (ex. 150.05 bpm is returned as 15005)
    uint32_t getDeckBPM();
    // get the current time of the deck/player
    uint32_t getDeckTime();
    // get the total time of the deck/player
    uint32_t getTotalTime();

private:
    uintptr_t find_device_offset(const char *name);
};

// lookup a djplayer/deck by index (0 to 3)
djplayer_uiplayer *lookup_player(uint32_t deck_idx);

// lookup the index of a player by pointer
uint32_t lookup_player_idx(djplayer_uiplayer *player);



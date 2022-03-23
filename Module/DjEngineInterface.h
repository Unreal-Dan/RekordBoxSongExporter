#pragma once

bool init_djengine_interface();

// shift key on a deck up from -12 to +12
bool adjust_deck_key(uint32_t deck, int key_offset);

// adjust the bpm some amount + or - bpms
bool adjust_deck_bpm(uint32_t deck, int tempo_offset);

// play and pause a deck
bool play_deck(uint32_t deck);
bool pause_deck(uint32_t deck);

// scratch/seek a deck, for example -2 is 2x backwards, 4 is 4x forwards
bool start_scratch(uint32_t deck, float magnitude);
bool stop_scratch(uint32_t deck);

// Mute/unmute a deck
bool mute_deck(uint32_t deck, bool mute);

// go back to current cue point and pause
bool back_to_current_cue(uint32_t deck);

// Slip loop
bool slip_loop(uint32_t deck);
bool stop_slip_loop(uint32_t deck);

// set play to true=forward, false=backwards. If backwards set temporary=true for a temporary change
bool set_play_direction(uint32_t deck, bool forward, bool temporary);

// set a channel fader to a volume amount
bool set_channel_fader(uint32_t deck, float amount);

// set the master output volume
bool set_master_volume(float amount);

// turn the mic on or off
bool set_mic_onoff(bool on);

#pragma once

// initialize the bpm control system
bool init_bpm_control();

// add some bpm amount to a deck
bool adjust_deck_bpm(uint32_t deck, int tempo_offset);

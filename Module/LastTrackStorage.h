#pragma once
#include <inttypes.h>

#include <string>

// must call this to initialize the storage
bool initialize_last_track_storage();

// store info in global threadsafe storage
void set_string_info(uint32_t deck, std::string key, std::string value);
void set_uint_info(uint32_t deck, std::string key, uint32_t value);
void set_float_info(uint32_t deck, std::string key, float value);

// whether this track has been logged
void set_logged(uint32_t deck, bool logged);

// fetch info from global threadsafe storage
std::string get_string_info(uint32_t deck, std::string key);
uint32_t get_uint_info(uint32_t deck, std::string key);
float get_float_info(uint32_t deck, std::string key);

// whether this track has been logged
bool get_logged(uint32_t deck);

// the current master index
void set_master(uint32_t);
uint32_t get_master();

// setters
#define set_last_title(deck, title)           set_string_info(deck, "title", title)
#define set_last_artist(deck, artist)         set_string_info(deck, "artist", artist)
#define set_last_album(deck, album)           set_string_info(deck, "album", album)
#define set_last_genre(deck, genre)           set_string_info(deck, "genre", genre)
#define set_last_label(deck, label)           set_string_info(deck, "label", label)
#define set_last_key(deck, key)               set_string_info(deck, "key", key)
#define set_last_orig_artist(deck, artist)    set_string_info(deck, "orig_artist", artist)
#define set_last_remixer(deck, remixer)       set_string_info(deck, "remixer", remixer)
#define set_last_composer(deck, composer)     set_string_info(deck, "composer", composer)
#define set_last_comment(deck, comment)       set_string_info(deck, "comment", comment)
#define set_last_mix_name(deck, mix_name)     set_string_info(deck, "mix_name", mix_name)
#define set_last_lyricist(deck, lyricist)     set_string_info(deck, "lyricist", lyricist)
#define set_last_date_created(deck, created)  set_string_info(deck, "date_created", created)
#define set_last_date_added(deck, added)      set_string_info(deck, "date_added", added)
#define set_last_track_number(deck, num)      set_uint_info(deck, "track_number", num)
#define set_last_bpm(deck, bpm)               set_float_info(deck, "bpm", bpm)

// getters
#define get_last_title(deck)                  get_string_info(deck, "title")
#define get_last_artist(deck)                 get_string_info(deck, "artist")
#define get_last_album(deck)                  get_string_info(deck, "album")
#define get_last_genre(deck)                  get_string_info(deck, "genre")
#define get_last_label(deck)                  get_string_info(deck, "label")
#define get_last_key(deck)                    get_string_info(deck, "key")
#define get_last_orig_artist(deck)            get_string_info(deck, "orig_artist")
#define get_last_remixer(deck)                get_string_info(deck, "remixer")
#define get_last_composer(deck)               get_string_info(deck, "composer")
#define get_last_comment(deck)                get_string_info(deck, "comment")
#define get_last_mix_name(deck)               get_string_info(deck, "mix_name")
#define get_last_lyricist(deck)               get_string_info(deck, "lyricist")
#define get_last_date_created(deck)           get_string_info(deck, "date_created")
#define get_last_date_added(deck)             get_string_info(deck, "date_added")
#define get_last_track_number(deck)           get_uint_info(deck, "track_number")
#define get_last_bpm(deck)                    get_float_info(deck, "bpm")

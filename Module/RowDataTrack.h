#pragma once

#include <Windows.h>
#include <inttypes.h>

// interface for version agnostic row data object
class row_data
{
public:
    const char *getTitle();
    const char *getArtist();
    const char *getAlbum();
    const char *getGenre();
    const char *getLabel();
    const char *getKey();
    const char *getOrigArtist();
    const char *getRemixer();
    const char *getComposer();
    const char *getComment();
    const char *getMixName();
    const char *getLyricist();
    const char *getDateCreated();
    const char *getDateAdded();
    uint32_t getTrackNumber();
    // note the bpm is an integer like 15150 to represent 151.50
    uint32_t getBpm();

protected:
    // retrieve member at offset
    template <typename T>
    T getValue(uint32_t offset) {
        return *reinterpret_cast<T*>((uintptr_t)this + offset);
    }
    // get a string value, cannot return NULL
    inline const char *getString(uint32_t offset) {
        const char *str = getValue<const char *>(offset);
        return str ? str : "";
    }
};

// initialize the rowdata lookup funcs
bool init_row_data_funcs();

// lookup a new rowdata object
row_data *lookup_row_data(uint32_t deck_idx);

// destroy a rowdata object
void destroy_row_data(row_data *rowdata);


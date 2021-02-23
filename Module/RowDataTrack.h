#pragma once

#include <inttypes.h>

// the track data for the row in the browser
struct row_data
{
    char pad_0000[16]; //0x0000
    char *uuid; //0x0010
    char pad_0018[8]; //0x0018
    char *title; //0x0020
    char *titleCaps; //0x0028
    char pad_0030[88]; //0x0030
    char *filePath; //0x0088
    char pad_0090[8]; //0x0090
    char *fileName; //0x0098
    char pad_00A0[32]; //0x00A0
    char *artist; //0x00C0
    char *aristCaps; //0x00C8
    char pad_00D0[40]; //0x00D0
    char *album; //0x00F8
    char *albumCaps; //0x0100
    char pad_0108[104]; //0x0108
    char *genre; //0x0170
    char *genreCaps; //0x0178
    char pad_0180[40]; //0x0180
    char *label; //0x01A8
    char *labelCaps; //0x01B0
    char pad_01B8[72]; //0x01B8
    char *key; //0x0200
    char pad_0208[120]; //0x0208
    char *org_artist; //0x0280
    char *origArtistCaps; //0x0288
    char pad_0290[40]; //0x0290
    char *remixer; //0x02B8
    char *remixerCaps; //0x02C0
    char pad_02C8[40]; //0x02C8
    char *composer; //0x02F0
    char *composerCaps; //0x02F8
    char pad_0300[8]; //0x0300
    uint32_t track_number; //0x0308
    char pad_030C[12]; //0x030C
    char *comment; //0x0318
    char *commentCaps; //0x0320
    char pad_0328[32]; //0x0328
    char *mix_name; //0x0348
    char *mixNameCaps; //0x0350
    char pad_0358[32]; //0x0358
    char *date_created; //0x0378
    char *date_added; //0x0380
    char pad_0388[24]; //0x0388
    char *artworkPath; //0x03A0
    char *artworkPathCaps; //0x03A8
    char pad_03B0[104]; //0x03B0
    char *lyricist; //0x0418
    char pad_0420[8]; //0x0420
    char *filePath2; //0x0428
    char pad_0430[40]; //0x0430
    char *dots1; //0x0458
    char *dots2; //0x0460
    char pad_0468[24]; //0x0468
    char *uuid2; //0x0480
};

// lookup a new rowdata object
row_data *lookup_row_data(uint32_t deck_idx);

// destroy a rowdata object
void destroy_row_data(row_data *rowdata);


#include <Windows.h>
#include <inttypes.h>

#include "RowDataTrack.h"
#include "UIPlayer.h"
#include "Config.h"
#include "Log.h"

// abusing __fastcall to simulate a __thiscall

// inst = db::DatabaseIF::getInstance();
typedef void *(__fastcall *get_instance_fn)();
// db::DatabaseIF::getRowDataTrack(v8, v7, &a1a, 1u, 0) 
typedef void *(__fastcall *get_rowdata_fn)(void *thisptr, unsigned int idx, void *outRowData, unsigned int a3, unsigned int a4);
// db::RowDataTrack::RowDataTrack(rowData);
typedef void *(__fastcall *init_rowdata_fn)(row_data *thisptr);
// db::RowDataTrack::~RowDataTrack(rowData);
typedef void *(__fastcall *destr_rowdata_fn)(row_data *thisptr);

// lookup a new rowdata object
row_data *lookup_row_data(uint32_t deck_idx)
{
    // the functions for looking up row data
    static get_instance_fn get_inst = (get_instance_fn)(rb_base() + 0x379000);
    static get_rowdata_fn get_rowdata = (get_rowdata_fn)(rb_base() + 0x10E3C30);
    static init_rowdata_fn init_rowdata = (init_rowdata_fn)(rb_base() + 0x371500);

    // lookup a player for the deck
    djplayer_uiplayer *uiplayer = lookup_player(deck_idx);
    // allocate a block of memory for the rowdata
    row_data *rowdata = (row_data *)calloc(1, 0x488);
    if (!rowdata) {
        error("Out of memory");
        return NULL;
    }
    // construct a RowDataTrack in the block of memory
    init_rowdata(rowdata);
    // call getRowDataTrack on the browserID of the given player
    // inst->getRowDataTrack(browserid, rowdata, 1, 0)
    get_rowdata(get_inst(), uiplayer->browserId, rowdata, 1, 0);  
    // return the new rowdata object
    return rowdata;
}

// destroy a rowdata object
void destroy_row_data(row_data *rowdata)
{
    // the destructor for rowdata
    static destr_rowdata_fn destr_rowdata = (destr_rowdata_fn)(rb_base() + 0x371B90);
    if (!rowdata) {
        return;
    }
    // destruct then free the block of memory
    destr_rowdata(rowdata); 
    free(rowdata);
}

#include <inttypes.h>

#include "RowDataTrack.h"
#include "UIPlayer.h"
#include "Config.h"
#include "Log.h"

// inst = db::DatabaseIF::getInstance();
typedef void *(*get_instance_fn)();
// db::DatabaseIF::getRowDataTrack(v8, v7, &a1a, 1u, 0) 
typedef void *(*get_rowdata_fn)(void *thisptr, unsigned int idx, void *outRowData, unsigned int a3, unsigned int a4);
// db::RowDataTrack::RowDataTrack(rowData);
typedef void *(*init_rowdata_fn)(void *thisptr);
// db::RowDataTrack::~RowDataTrack(rowData);
typedef void *(*destr_rowdata_fn)(void *thisptr);

// the functions for looking up row data
get_instance_fn get_inst = NULL;
get_rowdata_fn get_rowdata = NULL;
init_rowdata_fn init_rowdata = NULL;
// the destructor for rowdata
destr_rowdata_fn destr_rowdata = NULL;

bool init_row_data_funcs()
{
    // best way to find these addresses is look for _Loop.wav
    switch (config.rbox_version) {
    case RBVER_585:
        get_inst = (get_instance_fn)(rb_base() + 0x39F2F0);
        get_rowdata = (get_rowdata_fn)(rb_base() + 0xF28610);
        init_rowdata = (init_rowdata_fn)(rb_base() + 0x3963F0);
        destr_rowdata = (destr_rowdata_fn)(rb_base() + 0x396690);
        break;
    case RBVER_650:
        get_inst = (get_instance_fn)(rb_base() + 0x379000);
        get_rowdata = (get_rowdata_fn)(rb_base() + 0x10E3C30);
        init_rowdata = (init_rowdata_fn)(rb_base() + 0x371500);
        destr_rowdata = (destr_rowdata_fn)(rb_base() + 0x371B90);
        break;
    case RBVER_651:
        get_inst = (get_instance_fn)(rb_base() + 0x379410);
        get_rowdata = (get_rowdata_fn)(rb_base() + 0x10EF060);
        init_rowdata = (init_rowdata_fn)(rb_base() + 0x371910);
        destr_rowdata = (destr_rowdata_fn)(rb_base() + 0x371FA0);
        break;
    default:
        error("Invalid version");
    }
    return true;
}

// allocate a row data object
static row_data *new_row_data()
{
    void *rowdata = NULL;
    // okay yeah they're all the same but just to indicate it 
    // may change in a new version we separate into a switch
    // that the compiler will probably just optimize away 
    switch (config.rbox_version) {
    case RBVER_585:
        rowdata = calloc(1, 0x488);
        break;
    case RBVER_650:
        rowdata = calloc(1, 0x488);
        break;
    case RBVER_651:
        rowdata = calloc(1, 0x488);
        break;
    default:
        error("Invalid version");
        break;
    }
    return (row_data *)rowdata;
}

// lookup a new rowdata object
row_data *lookup_row_data(uint32_t deck_idx)
{
    // lookup a player for the deck
    djplayer_uiplayer *uiplayer = lookup_player(deck_idx);
    // allocate a block of memory for the rowdata
    row_data *rowdata = new_row_data();
    if (!rowdata) {
        error("Failed to allocate base rowdata object");
        return NULL;
    }
    // construct a RowDataTrack in the block of memory provided by the rowdata class
    init_rowdata(rowdata);
    // call getRowDataTrack on the browserID of the given player
    // inst->getRowDataTrack(browserid, rowdata, 1, 0)
    get_rowdata(get_inst(), uiplayer->getTrackBrowserID(), rowdata, 1, 0);  
    // return the new rowdata object
    return rowdata;
}

// destroy a rowdata object
void destroy_row_data(row_data *rowdata)
{
    if (!rowdata) {
        return;
    }
    // destruct then free the block of memory
    destr_rowdata(rowdata); 
    free(rowdata);
}

// the getter functions for the row data object
const char *row_data::getTitle()
{
    switch (config.rbox_version) {
    case RBVER_585: return getString(0x18);
    case RBVER_650: return getString(0x20);
    case RBVER_651: return getString(0x20);
    default:        return "";
    }
}
const char *row_data::getArtist()
{
    switch (config.rbox_version) {
    case RBVER_585: return getString(0xB0);
    case RBVER_650: return getString(0xC0);
    case RBVER_651: return getString(0xC0);
    default:        return "";
    }
}
const char *row_data::getAlbum()
{
    switch (config.rbox_version) {
    case RBVER_585: return getString(0xE0);
    case RBVER_650: return getString(0xF8);
    case RBVER_651: return getString(0xF8);
    default:        return "";
    }
}
const char *row_data::getGenre()
{
    switch (config.rbox_version) {
    case RBVER_585: return getString(0x148);
    case RBVER_650: return getString(0x170);
    case RBVER_651: return getString(0x170);
    default:        return "";
    }
}
const char *row_data::getLabel()
{
    switch (config.rbox_version) {
    case RBVER_585: return getString(0x178);
    case RBVER_650: return getString(0x1A8);
    case RBVER_651: return getString(0x1A8);
    default:        return "";
    }
}
const char *row_data::getKey()
{
    switch (config.rbox_version) {
    case RBVER_585: return getString(0x1C8);
    case RBVER_650: return getString(0x200);
    case RBVER_651: return getString(0x200);
    default:        return "";
    }
}
const char *row_data::getOrigArtist()
{
    switch (config.rbox_version) {
    case RBVER_585: return getString(0x238);
    case RBVER_650: return getString(0x280);
    case RBVER_651: return getString(0x280);
    default:        return "";
    }
}
const char *row_data::getRemixer()
{
    switch (config.rbox_version) {
    case RBVER_585: return getString(0x268);
    case RBVER_650: return getString(0x2B8);
    case RBVER_651: return getString(0x2B8);
    default:        return "";
    }
}
const char *row_data::getComposer()
{
    switch (config.rbox_version) {
    case RBVER_585: return getString(0x298);
    case RBVER_650: return getString(0x2F0);
    case RBVER_651: return getString(0x2F0);
    default:        return "";
    }
}
const char *row_data::getComment()
{
    switch (config.rbox_version) {
    case RBVER_585: return getString(0x2C0);
    case RBVER_650: return getString(0x318);
    case RBVER_651: return getString(0x318);
    default:        return "";
    }
}
const char *row_data::getMixName()
{
    switch (config.rbox_version) {
    case RBVER_585: return getString(0x2F0);
    case RBVER_650: return getString(0x348);
    case RBVER_651: return getString(0x348);
    default:        return "";
    }
}
const char *row_data::getLyricist()
{
    switch (config.rbox_version) {
    case RBVER_585: return getString(0x3B8);
    case RBVER_650: return getString(0x418);
    case RBVER_651: return getString(0x418);
    default:        return "";
    }
}
const char *row_data::getDateCreated()
{
    switch (config.rbox_version) {
    case RBVER_585: return getString(0x320);
    case RBVER_650: return getString(0x378);
    case RBVER_651: return getString(0x378);
    default:        return "";
    }
}
const char *row_data::getDateAdded()
{
    switch (config.rbox_version) {
    case RBVER_585: return getString(0x328);
    case RBVER_650: return getString(0x380);
    case RBVER_651: return getString(0x380);
    default:        return "";
    }
}
uint32_t row_data::getTrackNumber()
{
    switch (config.rbox_version) {
    case RBVER_585: return getValue<uint32_t>(0x2B0);
    case RBVER_650: return getValue<uint32_t>(0x308);
    case RBVER_651: return getValue<uint32_t>(0x308);
    default:        return 0;
    }
}
uint32_t row_data::getBpm()
{
    switch (config.rbox_version) {
    case RBVER_585: return getValue<uint32_t>(0x308);
    case RBVER_650: return getValue<uint32_t>(0x360);
    case RBVER_651: return getValue<uint32_t>(0x360);
    default:        return 0;
    }
}


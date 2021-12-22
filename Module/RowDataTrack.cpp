#include <inttypes.h>

#include "RowDataTrack.h"
#include "UIPlayer.h"
#include "SigScan.h"
#include "Config.h"
#include "Log.h"

// db::databaseif::getinstance();
#define GET_INSTANCE_SIG "\x40\x57\x48\x83\xec\x30\x48\xc7\x44\x24\x20\xfe\xff\xff\xff\x48\x89\x5c\x24\x58\x48\x8b\x05\x90\x90\x90\x90\x48\x85\xc0"
#define GET_INSTANCE_SIG_LEN (sizeof(GET_INSTANCE_SIG) - 1)
// db::databaseif::getrowdatatrack(v8, v7, &a1a, 1u, 0) 
#define GET_ROW_DATA_TRACK_SIG "\x48\x89\x5c\x24\x08\x57\x48\x83\xec\x20\x48\x8b\x09\x49\x8b\xd8\x44\x8b\x44\x24\x50\x8b\xfa\x41\x8b\xd1"
#define GET_ROW_DATA_TRACK_SIG_LEN (sizeof(GET_ROW_DATA_TRACK_SIG) - 1)
// db::rowdatatrack::rowdatatrack(rowdata);
#define ROW_DATA_TRACK_SIG "\x48\x89\x4c\x24\x08\x53\x55\x56\x57\x41\x56\x48\x83\xec\x30\x48\xc7\x44\x24\x20\xfe\xff\xff\xff\x48\x8b\xf9\xbd\x01\x00\x00\x00"
#define ROW_DATA_TRACK_SIG_LEN (sizeof(ROW_DATA_TRACK_SIG) - 1)
// db::rowdatatrack::~rowdatatrack(rowdata);
#define DESTR_ROW_DATA_SIG "\x48\x89\x4c\x24\x08\x53\x55\x56\x57\x41\x56\x48\x83\xec\x30\x48\xc7\x44\x24\x20\xfe\xff\xff\xff\x4c\x8b\xf1\x48\x8d\x05\x90\x90\x90\x90\x48\x89\x01\x48"
#define DESTR_ROW_DATA_SIG_LEN (sizeof(DESTR_ROW_DATA_SIG) - 1)

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
    // best way to find these addresses is look for "_Loop.wav" then scroll up 
    // to where there are several casts and something that looks like this: 
    //
    //     if ( v284 == 1 || (unsigned int)(v284 - 2) <= 1 )
    //       v285 = *(_DWORD *)(v283 + 32);
    //     db::RowDataTrack::RowDataTrack((__int64)&v560);      // 3rd function
    //     v286 = db::DatabaseIF::getInstance();                // 1st function
    //     LODWORD(v530) = 0;
    //     if ( db::DatabaseIF::getRowDataTrack(v286) )         // 2nd function
    //     {
    //       sub_1622970(&v547, &v561);
    //       sub_1622970(&v546, &v562);
    //       sub_1622970(&v540, &v563);
    //       sub_1622970(&v545, &v565);
    //       sub_1622970(&v551, &v564);
    //     }
    //     db::RowDataTrack::DestructRowDataTrack(&v560);       // 4th function
    switch (config.version) {
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
    case RBVER_652:
        get_inst = (get_instance_fn)(rb_base() + 0x381650);
        get_rowdata = (get_rowdata_fn)(rb_base() + 0x10FDC20);
        init_rowdata = (init_rowdata_fn)(rb_base() + 0x379AB0);
        destr_rowdata = (destr_rowdata_fn)(rb_base() + 0x37A140);
        break;
    case RBVER_653:
        get_inst = (get_instance_fn)(rb_base() + 0x249550);
        get_rowdata = (get_rowdata_fn)(rb_base() + 0xFD74D0);
        init_rowdata = (init_rowdata_fn)(rb_base() + 0x2419B0);
        destr_rowdata = (destr_rowdata_fn)(rb_base() + 0x242040);
        break;
    default: // RBVER_661+
        // sig scan for each function
        get_inst = (get_instance_fn)sig_scan(NULL, GET_INSTANCE_SIG, GET_INSTANCE_SIG_LEN);
        get_rowdata = (get_rowdata_fn)sig_scan(NULL, GET_ROW_DATA_TRACK_SIG, GET_ROW_DATA_TRACK_SIG_LEN);
        init_rowdata = (init_rowdata_fn)sig_scan(NULL, ROW_DATA_TRACK_SIG, ROW_DATA_TRACK_SIG_LEN);
        destr_rowdata = (destr_rowdata_fn)sig_scan(NULL, DESTR_ROW_DATA_SIG, DESTR_ROW_DATA_SIG_LEN);
        break;
    }
    if (!get_inst) {
        error("Failed to locate db::DatabaseIF::getInstance()");
        return false;
    }
    if (!get_rowdata) {
        error("Failed to locate db::DatabaseIF::getRowDataTrack()");
        return false;
    }
    if (!init_rowdata) {
        error("Failed to locate db::RowDataTrack::RowDataTrack()");
        return false;
    }
    if (!destr_rowdata) {
        error("Failed to locate db::RowDataTrack::~RowDataTrack()");
        return false;
    }
    success("Found get_inst: %p", get_inst);
    success("Found get_rowdata: %p", get_rowdata);
    success("Found init_rowdata: %p", init_rowdata);
    success("Found destr_rowdata: %p", destr_rowdata);
    return true;
}

// allocate a row data object
static row_data *new_row_data()
{
    void *rowdata = NULL;
    // In order to find the size of the RowDataTrack you must find a call like this:
    //
    //   LODWORD(v8) = sub_1049668(0x488i64);               // this is the allocator
    //   LODWORD(v9) = db::RowDataTrack::RowDataTrack(v8);  // this is the constructor
    //
    // That second call is the 'init_rowdata' address resolved in the above function,
    // it is called in many places but only sometimes called with malloc()'d memory.
    // You will need to search to find a call to db::RowDataTrack::RowDataTrack() that 
    // has a call to the allocator directly above it.
    //
    // The 0x488 is the size of the RowDataTrack object being allocated, so far I have
    // not observed any differences in the size of the RowDataTrack but it could change
    // in a new version so I used a switch statement in case future versions differ.
    switch (config.version) {
    case RBVER_585:
    case RBVER_650:
    case RBVER_651:
    case RBVER_652:
    case RBVER_653:
    case RBVER_661:
    default: // RBVER_661+ just guess
        rowdata = calloc(1, 0x488);
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
    switch (config.version) {
    case RBVER_585: return getString(0x18);
    case RBVER_650: return getString(0x20);
    case RBVER_651: return getString(0x20);
    case RBVER_652: return getString(0x20);
    case RBVER_653: return getString(0x20);
    default:        
        if (config.version >= RBVER_661) {
            return getString(0x20);
        }
        return "";
    }
}
const char *row_data::getArtist()
{
    switch (config.version) {
    case RBVER_585: return getString(0xB0);
    case RBVER_650: return getString(0xC0);
    case RBVER_651: return getString(0xC0);
    case RBVER_652: return getString(0xC0);
    case RBVER_653: return getString(0xC0);
    case RBVER_661: return getString(0xC0);
    default:        
        if (config.version >= RBVER_661) {
            return getString(0xC0);
        }
        return "";
    }
}
const char *row_data::getAlbum()
{
    switch (config.version) {
    case RBVER_585: return getString(0xE0);
    case RBVER_650: return getString(0xF8);
    case RBVER_651: return getString(0xF8);
    case RBVER_652: return getString(0xF8);
    case RBVER_653: return getString(0xF8);
    case RBVER_661: return getString(0xF8);
    default:        
        if (config.version >= RBVER_661) {
            return getString(0xF8);
        }
        return "";
    }
}
const char *row_data::getGenre()
{
    switch (config.version) {
    case RBVER_585: return getString(0x148);
    case RBVER_650: return getString(0x170);
    case RBVER_651: return getString(0x170);
    case RBVER_652: return getString(0x170);
    case RBVER_653: return getString(0x170);
    case RBVER_661: return getString(0x170);
    default:        
        if (config.version >= RBVER_661) {
            return getString(0x170);
        }
        return "";
    }
}
const char *row_data::getLabel()
{
    switch (config.version) {
    case RBVER_585: return getString(0x178);
    case RBVER_650: return getString(0x1A8);
    case RBVER_651: return getString(0x1A8);
    case RBVER_652: return getString(0x1A8);
    case RBVER_653: return getString(0x1A8);
    case RBVER_661: return getString(0x1A8);
    default:        
        if (config.version >= RBVER_661) {
            return getString(0x1A8);
        }
        return "";
    }
}
const char *row_data::getKey()
{
    switch (config.version) {
    case RBVER_585: return getString(0x1C8);
    case RBVER_650: return getString(0x200);
    case RBVER_651: return getString(0x200);
    case RBVER_652: return getString(0x200);
    case RBVER_653: return getString(0x200);
    case RBVER_661: return getString(0x200);
    default:        
        if (config.version >= RBVER_661) {
            return getString(0x200);
        }
        return "";
    }
}
const char *row_data::getOrigArtist()
{
    switch (config.version) {
    case RBVER_585: return getString(0x238);
    case RBVER_650: return getString(0x280);
    case RBVER_651: return getString(0x280);
    case RBVER_652: return getString(0x280);
    case RBVER_653: return getString(0x280);
    case RBVER_661: return getString(0x280);
    default:        
        if (config.version >= RBVER_661) {
            return getString(0x280);
        }
        return "";
    }
}
const char *row_data::getRemixer()
{
    switch (config.version) {
    case RBVER_585: return getString(0x268);
    case RBVER_650: return getString(0x2B8);
    case RBVER_651: return getString(0x2B8);
    case RBVER_652: return getString(0x2B8);
    case RBVER_653: return getString(0x2B8);
    case RBVER_661: return getString(0x2B8);
    default:        
        if (config.version >= RBVER_661) {
            return getString(0x2B8);
        }
        return "";
    }
}
const char *row_data::getComposer()
{
    switch (config.version) {
    case RBVER_585: return getString(0x298);
    case RBVER_650: return getString(0x2F0);
    case RBVER_651: return getString(0x2F0);
    case RBVER_652: return getString(0x2F0);
    case RBVER_653: return getString(0x2F0);
    case RBVER_661: return getString(0x2F0);
    default:        
        if (config.version >= RBVER_661) {
            return getString(0x2F0);
        }
        return "";
    }
}
const char *row_data::getComment()
{
    switch (config.version) {
    case RBVER_585: return getString(0x2C0);
    case RBVER_650: return getString(0x318);
    case RBVER_651: return getString(0x318);
    case RBVER_652: return getString(0x318);
    case RBVER_653: return getString(0x318);
    case RBVER_661: return getString(0x318);
    default:        
        if (config.version >= RBVER_661) {
            return getString(0x318);
        }
        return "";
    }
}
const char *row_data::getMixName()
{
    switch (config.version) {
    case RBVER_585: return getString(0x2F0);
    case RBVER_650: return getString(0x348);
    case RBVER_651: return getString(0x348);
    case RBVER_652: return getString(0x348);
    case RBVER_653: return getString(0x348);
    case RBVER_661: return getString(0x348);
    default:        
        if (config.version >= RBVER_661) {
            return getString(0x348);
        }
        return "";
    }
}
const char *row_data::getLyricist()
{
    switch (config.version) {
    case RBVER_585: return getString(0x3B8);
    case RBVER_650: return getString(0x418);
    case RBVER_651: return getString(0x418);
    case RBVER_652: return getString(0x418);
    case RBVER_653: return getString(0x418);
    case RBVER_661: return getString(0x418);
    default:        
        if (config.version >= RBVER_661) {
            return getString(0x418);
        }
        return "";
    }
}
const char *row_data::getDateCreated()
{
    switch (config.version) {
    case RBVER_585: return getString(0x320);
    case RBVER_650: return getString(0x378);
    case RBVER_651: return getString(0x378);
    case RBVER_652: return getString(0x378);
    case RBVER_653: return getString(0x378);
    case RBVER_661: return getString(0x378);
    default:        
        if (config.version >= RBVER_661) {
            return getString(0x378);
        }
        return "";
    }
}
const char *row_data::getDateAdded()
{
    switch (config.version) {
    case RBVER_585: return getString(0x328);
    case RBVER_650: return getString(0x380);
    case RBVER_651: return getString(0x380);
    case RBVER_652: return getString(0x380);
    case RBVER_653: return getString(0x380);
    case RBVER_661: return getString(0x380);
    default:        
        if (config.version >= RBVER_661) {
            return getString(0x380);
        }
        return "";
    }
}
uint32_t row_data::getTrackNumber()
{
    switch (config.version) {
    case RBVER_585: return getValue<uint32_t>(0x2B0);
    case RBVER_650: return getValue<uint32_t>(0x308);
    case RBVER_651: return getValue<uint32_t>(0x308);
    case RBVER_652: return getValue<uint32_t>(0x308);
    case RBVER_653: return getValue<uint32_t>(0x308);
    case RBVER_661: return getValue<uint32_t>(0x308);
    default:        
        if (config.version >= RBVER_661) {
            return getValue<uint32_t>(0x308);
        }
        return 0;
    }
}
uint32_t row_data::getBpm()
{
    switch (config.version) {
    case RBVER_585: return getValue<uint32_t>(0x308);
    case RBVER_650: return getValue<uint32_t>(0x360);
    case RBVER_651: return getValue<uint32_t>(0x360);
    case RBVER_652: return getValue<uint32_t>(0x360);
    case RBVER_653: return getValue<uint32_t>(0x360);
    case RBVER_661: return getValue<uint32_t>(0x360);
    default:        
        if (config.version >= RBVER_661) {
            return getValue<uint32_t>(0x360);
        }
        return 0;
    }
}

#include <inttypes.h>

#include "LastTrackStorage.h"
#include "RowDataTrack.h"
#include "UIPlayer.h"
#include "SigScan.h"
#include "Config.h"
#include "Log.h"

// best way to find these signatures is look for "_Loop.wav" then scroll up
// to where there are several casts and something that looks like this:
//
//     if ( v284 == 1 || (unsigned int)(v284 - 2) <= 1 )    // look for this line
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

// db::databaseif::getinstance();
#define GET_INSTANCE_SIG "40 57 48 83 ec 30 48 c7 44 24 20 fe ff ff ff 48 89 5c 24 58 48 8b 05 90 90 90 90 48 85 c0"
// db::databaseif::getrowdatatrack(v8, v7, &a1a, 1u, 0)
#define GET_ROW_DATA_TRACK_SIG "48 89 5c 24 08 57 48 83 ec 20 48 8b 09 49 8b d8 44 8b 44 24 50 8b fa 41 8b d1"
// db::rowdatatrack::rowdatatrack(rowdata);
#define ROW_DATA_TRACK_SIG "48 89 4c 24 08 53 55 56 57 41 56 48 83 ec 30 48 c7 44 24 20 fe ff ff ff 48 8b f9 bd 01 00 00 00"
// db::rowdatatrack::~rowdatatrack(rowdata);
#define DESTR_ROW_DATA_SIG "48 89 4c 24 08 53 55 56 57 41 56 48 83 ec 30 48 c7 44 24 20 fe ff ff ff 4c 8b f1 48 8d 05 90 90 90 90 48 89 01 48"

// same sigs for 7.0.1:
#define GET_INSTANCE_SIG_701 "48 89 5c 24 18 57 48 83 ec 30 48 8b 05 90 90 90 90 90 48 85 c0 0f 85 a2"
// this is actually a sig for an inner version of getRowDataTrack, just deref rcx/this first
#define GET_ROW_DATA_TRACK_SIG_701 "48 89 5c 24 08 57 48 83 ec 20 49 8b d8 8b fa 44"
#define ROW_DATA_TRACK_SIG_701 "48 89 5c 24 18 48 89 4c 24 08 55 56 57 48 83 ec 20 48 8b f1"
#define DESTR_ROW_DATA_SIG_701 "48 89 5c 24 08 57 48 83 ec 20 48 8d 05 90 90 90 90 48 8b d9 48 89 01 48 8d 05 90 90 90 90 48 89 41 38 48 81 c1 c0 04 00 00 e8 90 90 90 90 48"

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
    case RBVER_661: // 6.6.1
    case RBVER_662: // 6.6.2
    case RBVER_663: // 6.6.3
    case RBVER_664: // 6.6.4
    case RBVER_6610: // 6.6.10
    case RBVER_6611: // 6.6.11
    case RBVER_670: // 6.7.0
    case RBVER_675: // 6.7.5
        get_inst = (get_instance_fn)sig_scan(GET_INSTANCE_SIG);
        get_rowdata = (get_rowdata_fn)sig_scan(GET_ROW_DATA_TRACK_SIG);
        init_rowdata = (init_rowdata_fn)sig_scan(ROW_DATA_TRACK_SIG);
        destr_rowdata = (destr_rowdata_fn)sig_scan(DESTR_ROW_DATA_SIG);
        break;
    case RBVER_708:
    default: // RBVER_701+
        get_inst = (get_instance_fn)sig_scan(GET_INSTANCE_SIG_701);
        get_rowdata = (get_rowdata_fn)sig_scan(GET_ROW_DATA_TRACK_SIG_701);
        init_rowdata = (init_rowdata_fn)sig_scan(ROW_DATA_TRACK_SIG_701);
        destr_rowdata = (destr_rowdata_fn)sig_scan(DESTR_ROW_DATA_SIG_701);
        // sig scan for each function
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
        // this used to be 0x488 but it was crashing so I upped it to 0x512, should be enough
        // I later upped it to 0x1024 because why the hell not screw memory
        rowdata = calloc(1, 0x1024);
        break;
    }
    return (row_data *)rowdata;
}

// lookup a new rowdata object
row_data *lookup_row_data(uint32_t deck_idx)
{
    // allocate a block of memory for the rowdata
    row_data *rowdata = new_row_data();
    if (!rowdata) {
        error("Failed to allocate base rowdata object");
        return NULL;
    }
    // construct a RowDataTrack in the block of memory provided by the rowdata class
    init_rowdata(rowdata);

    // in rbver 701+ the getRowDataTrack function has a tiny thunk wrapper which derefernces rcx
    // so instead of sigging that func because it's 2 opcodes, we sig the inner func and just deref
    // rcx before calling it ourselves
    void *inst = nullptr;
    if (config.version >= RBVER_708) {
      inst = *(void **)get_inst();
    } else {
      inst = get_inst();
    }
    uint32_t trackid = get_track_id(deck_idx);
    if (!trackid) {
      destroy_row_data(rowdata);
      // I don't think the trackid should ever be 0 but idk
      return nullptr;
    }
    // call getRowDataTrack on the browserID of the given player
    // inst->getRowDataTrack(browserid, rowdata, 1, 0)
    get_rowdata(inst, trackid, rowdata, 1, 0);
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
    case RBVER_585:
      return getString(0x18);
    case RBVER_650:
    case RBVER_651:
    case RBVER_652:
    case RBVER_653:
    case RBVER_661:
    case RBVER_662:
    case RBVER_663:
    case RBVER_664:
    case RBVER_6610:
    case RBVER_6611:
    case RBVER_670:
    case RBVER_675:
      return getString(0x20);
    case RBVER_708:
    default:
      return getString(0x20);
    }
}
const char *row_data::getArtist()
{
    switch (config.version) {
    case RBVER_585:
      return getString(0xB0);
    case RBVER_650:
    case RBVER_651:
    case RBVER_652:
    case RBVER_653:
    case RBVER_661:
    case RBVER_662:
    case RBVER_663:
    case RBVER_664:
    case RBVER_6610:
    case RBVER_6611:
    case RBVER_670:
    case RBVER_675:
      return getString(0xC0);
    case RBVER_708:
    default:
      return getString(0xC0);
    }
}
const char *row_data::getAlbum()
{
    switch (config.version) {
    case RBVER_585:
      return getString(0xE0);
    case RBVER_650:
    case RBVER_651:
    case RBVER_652:
    case RBVER_653:
    case RBVER_661:
    case RBVER_662:
    case RBVER_663:
    case RBVER_664:
    case RBVER_6610:
    case RBVER_6611:
    case RBVER_670:
    case RBVER_675:
      return getString(0xF8);
    case RBVER_708:
    default:
      return getString(0xF8);
    }
}
const char *row_data::getGenre()
{
    switch (config.version) {
    case RBVER_585:
      return getString(0x148);
    case RBVER_650:
    case RBVER_651:
    case RBVER_652:
    case RBVER_653:
    case RBVER_661:
    case RBVER_662:
    case RBVER_663:
    case RBVER_664:
    case RBVER_6610:
    case RBVER_6611:
    case RBVER_670:
    case RBVER_675:
      return getString(0x170);
    case RBVER_708:
    default:
      return getString(0x170);
    }
}
const char *row_data::getLabel()
{
    switch (config.version) {
    case RBVER_585:
      return getString(0x178);
    case RBVER_650:
    case RBVER_651:
    case RBVER_652:
    case RBVER_653:
    case RBVER_661:
    case RBVER_662:
    case RBVER_663:
    case RBVER_664:
    case RBVER_6610:
    case RBVER_6611:
    case RBVER_670:
    case RBVER_675:
      return getString(0x1A8);
    case RBVER_708:
    default:
      return getString(0x1A8);
    }
}
const char *row_data::getKey()
{
    switch (config.version) {
    case RBVER_585:
      return getString(0x1C8);
    case RBVER_650:
    case RBVER_651:
    case RBVER_652:
    case RBVER_653:
    case RBVER_661:
    case RBVER_662:
    case RBVER_663:
    case RBVER_664:
    case RBVER_6610:
    case RBVER_6611:
    case RBVER_670:
    case RBVER_675:
      return getString(0x200);
    case RBVER_708:
    default:
      return getString(0x200);
    }
}
const char *row_data::getOrigArtist()
{
    switch (config.version) {
    case RBVER_585:
      return getString(0x238);
    case RBVER_650:
    case RBVER_651:
    case RBVER_652:
    case RBVER_653:
    case RBVER_661:
    case RBVER_662:
    case RBVER_663:
    case RBVER_664:
    case RBVER_6610:
    case RBVER_6611:
    case RBVER_670:
    case RBVER_675:
      return getString(0x280);
    case RBVER_708:
    default:
      return getString(0x280);
    }
}
const char *row_data::getRemixer()
{
    switch (config.version) {
    case RBVER_585:
      return getString(0x268);
    case RBVER_650:
    case RBVER_651:
    case RBVER_652:
    case RBVER_653:
    case RBVER_661:
    case RBVER_662:
    case RBVER_663:
    case RBVER_664:
    case RBVER_6610:
    case RBVER_6611:
    case RBVER_670:
    case RBVER_675:
      return getString(0x2B8);
    case RBVER_708:
    default:
      return getString(0x2F0);
    }
}
const char *row_data::getComposer()
{
    switch (config.version) {
    case RBVER_585:
      return getString(0x298);
    case RBVER_650:
    case RBVER_651:
    case RBVER_652:
    case RBVER_653:
    case RBVER_661:
    case RBVER_662:
    case RBVER_663:
    case RBVER_664:
    case RBVER_6610:
    case RBVER_6611:
    case RBVER_670:
    case RBVER_675:
      return getString(0x2F0);
    case RBVER_708:
    default:
      return getString(0x328);
    }
}
const char *row_data::getComment()
{
    switch (config.version) {
    case RBVER_585:
      return getString(0x2C0);
    case RBVER_650:
    case RBVER_651:
    case RBVER_652:
    case RBVER_653:
    case RBVER_661:
    case RBVER_662:
    case RBVER_663:
    case RBVER_664:
    case RBVER_6610:
    case RBVER_6611:
    case RBVER_670:
    case RBVER_675:
      return getString(0x318);
    case RBVER_708:
    default:
      return getString(0x350);
    }
}
const char *row_data::getMixName()
{
    switch (config.version) {
    case RBVER_585:
      return getString(0x2F0);
    case RBVER_650:
    case RBVER_651:
    case RBVER_652:
    case RBVER_653:
    case RBVER_661:
    case RBVER_662:
    case RBVER_663:
    case RBVER_664:
    case RBVER_6610:
    case RBVER_6611:
    case RBVER_670:
    case RBVER_675:
      return getString(0x348);
    case RBVER_708:
    default:
      return getString(0x380);
    }
}
const char *row_data::getLyricist()
{
    switch (config.version) {
    case RBVER_585:
      return getString(0x3B8);
    case RBVER_650:
    case RBVER_651:
    case RBVER_652:
    case RBVER_653:
    case RBVER_661:
    case RBVER_662:
    case RBVER_663:
    case RBVER_664:
    case RBVER_6610:
    case RBVER_6611:
    case RBVER_670:
    case RBVER_675:
      return getString(0x418);
    case RBVER_708:
    default:
      return getString(0x448);
    }
}
const char *row_data::getDateCreated()
{
    switch (config.version) {
    case RBVER_585:
      return getString(0x320);
    case RBVER_650:
    case RBVER_651:
    case RBVER_652:
    case RBVER_653:
    case RBVER_661:
    case RBVER_662:
    case RBVER_663:
    case RBVER_664:
    case RBVER_6610:
    case RBVER_6611:
    case RBVER_670:
    case RBVER_675:
      return getString(0x378);
    case RBVER_708:
    default:
      return getString(0x3b8);
    }
}
const char *row_data::getDateAdded()
{
    switch (config.version) {
    case RBVER_585:
      return getString(0x328);
    case RBVER_650:
    case RBVER_651:
    case RBVER_652:
    case RBVER_653:
    case RBVER_661:
    case RBVER_662:
    case RBVER_663:
    case RBVER_664:
    case RBVER_6610:
    case RBVER_6611:
    case RBVER_670:
    case RBVER_675:
      return getString(0x380);
    case RBVER_708:
    default:
      return getString(0x3b8);
    }
}
uint32_t row_data::getTrackNumber()
{
    switch (config.version) {
    case RBVER_585:
      return getValue<uint32_t>(0x2B0);
    case RBVER_650:
    case RBVER_651:
    case RBVER_652:
    case RBVER_653:
    case RBVER_661:
    case RBVER_662:
    case RBVER_663:
    case RBVER_664:
    case RBVER_6610:
    case RBVER_6611:
    case RBVER_670:
    case RBVER_675:
      return getValue<uint32_t>(0x308);
    case RBVER_708:
    default:
      return getValue<uint32_t>(0x340);
    }
}
uint32_t row_data::getBpm()
{
    switch (config.version) {
    case RBVER_585:
      return getValue<uint32_t>(0x308);
    case RBVER_650:
    case RBVER_651:
    case RBVER_652:
    case RBVER_653:
    case RBVER_661:
    case RBVER_662:
    case RBVER_663:
    case RBVER_664:
    case RBVER_6610:
    case RBVER_6611:
    case RBVER_670:
    case RBVER_675:
      return getValue<uint32_t>(0x360);
    case RBVER_708:
    default:
      return getValue<uint32_t>(0x398);
    }
}

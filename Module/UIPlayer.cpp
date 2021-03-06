#include <Windows.h>
#include <inttypes.h>

#include "UIPlayer.h"
#include "Config.h"
#include "Log.h"

// get the ID of the track which is used to lookup track info in the browser
uint32_t djplayer_uiplayer::getTrackBrowserID()
{
    // probably only changes on major versions
    // I found this by using ReClass to inspect the UiPlayer Object and
    // I would change the track and see which members changed. I found
    // some sort of ID that was changing and it turned out to be the ID
    // that I could pass to GetRowDataTrack. Unfortunately there is no
    // easy way of finding this offset again other than using ReClass to
    // look at the UiPlayer object and see what changes when tracks load.
    switch (config.version) {
    case RBVER_585:
        return *(uint32_t *)((uintptr_t)this + 0x3A0);
    case RBVER_650:
    case RBVER_651:
    case RBVER_652:
        return *(uint32_t *)((uintptr_t)this + 0x368);
    default:
        error("Unknown version");
        break;
    }
    return 0;
}

// lookup a djplayer/deck by index (0 to 3)
djplayer_uiplayer *lookup_player(uint32_t deck_idx) 
{
    // there's only 4 decks in this object
    if (deck_idx > 4) {
        deck_idx = 0;
    }
    // The main component is the global object that contains the UIManager object.
    // Inside the UIManager object is four consecutive UIPlayer pointers.
    //
    // To find the main component address look for "MainComponent", all three
    // occurrences of the string will be the constructor of the MainComponent. 
    //
    // The third occurrence will be shortly after the new object has been zeroed 
    // out and the global MainComponent will be initialized like this:
    //
    //    *(_WORD *)(v3 + 3144) = 0;
    //    *(_WORD *)(v3 + 3147) = 0;
    //    *(_QWORD *)(v3 + 3152) = 0i64;
    //    *(_QWORD *)(v3 + 3160) = 0i64;
    //    *(_QWORD *)(v3 + 3168) = 0i64;
    //    gMainComponent = v3;              // The main component global 
    //    sub_172A060(*(_QWORD *)(v3 + 1512), v3 + 1056);
    //
    // Just find v3 being assigned right after lots of members are set to 0.
    //
    // The second xref of gMainComponent should be a simple function returning
    // the pointer, this is MainComponent::getInstance()
    //
    // To find the offset for the UIManager look for string "Layout" and the first 
    // occurrence should be shortly before a usage of MainComponent::getInstance()
    // the call right after utilizes the UIManager to call UIManager->InitSkin:
    //
    //    v209 = MainComponent::getInstance();
    //    djplay::UiManager::initSkin(*(v209 + 0x648), &v498[4], 0);
    //                                         ^ This is the UIManager offset
    //
    // Then finally to get the pPlayers offset look for "WindowID" and at the 
    // first occurrence, the bottom of the function looks like this:
    //
    //      djplay::UiManager::createObject(v8);
    //      *(_QWORD *)(v1 + 1608) = v8;
    //    }
    //    return sub_1049A00((unsigned __int64)&v17 ^ v26);
    // 
    // At the start of djplay::UiManager::createObject() is assignments like this:
    //
    //    v4 = (__int64 *)(v1 + 0x50);
    //                          ^ the first UIPlayer offset
    //
    // This is the first UiPlayer of four UiPlayers. It's not an array, they are 
    // just consecutive members but they can be accessed like an array.
    uintptr_t main_component;
    uintptr_t ui_manager;
    djplayer_uiplayer **pPlayers;
    switch (config.version) {
    case RBVER_585:
        main_component = *(uintptr_t *)(rb_base() + 0x39D05D0);
        ui_manager = *(uintptr_t *)(main_component + 0x638);
        pPlayers = (djplayer_uiplayer **)(ui_manager + 0x50);
        break;
    case RBVER_650:
        main_component = *(uintptr_t *)(rb_base() + 0x3F38108);
        ui_manager = *(uintptr_t *)(main_component + 0x648);
        pPlayers = (djplayer_uiplayer **)(ui_manager + 0x50);
        break;
    case RBVER_651:
        main_component = *(uintptr_t *)(rb_base() + 0x3F649E8);
        ui_manager = *(uintptr_t *)(main_component + 0x648);
        pPlayers = (djplayer_uiplayer **)(ui_manager + 0x50);
        break;
    case RBVER_652:
        main_component = *(uintptr_t *)(rb_base() + 0x3F8B318);
        ui_manager = *(uintptr_t *)(main_component + 0x648);
        pPlayers = (djplayer_uiplayer **)(ui_manager + 0x50);
        break;
    default:
        error("Unknown version");
        return NULL;
    }
    // pPlayers is really just the first of four consecutive UIPlayers in
    // the UIManager object so we can access them like an array
    return pPlayers[deck_idx];
}


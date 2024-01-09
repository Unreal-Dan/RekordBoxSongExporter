#include <Windows.h>
#include <inttypes.h>

#include "UIPlayer.h"
#include "SigScan.h"
#include "Config.h"
#include "Log.h"

// sig for a function that accesses the global g_MainComponent object
#define MAIN_COMPONENT_SIG "49 8B CE 41 2B FC 41 2B FF 2B FD 2B BC 24 C0 00 00 00 41 2B FD"

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
  case RBVER_653:
  default:
    // use this offset for all versions after 650
    if (config.version >= RBVER_650) {
      return *(uint32_t *)((uintptr_t)this + 0x368);
    }
    error("Unknown version");
    break;
  }
  return 0;
}

uintptr_t djplayer_uiplayer::find_device_offset(const char *name)
{
  // The start of the device list shouldn't change, but it might
  // To find the start of the device list just look at the djplayer object
  // in reclass and see where the massive list of pointers begins
  uintptr_t device_list_start = (uintptr_t)this + 0x450;
  uintptr_t device_list_end = (uintptr_t)this + 0xe60;
  uintptr_t device_ptr = device_list_start;
  // iterate the device list and find the device with the name @BPM
  do {
    uintptr_t device = *(uintptr_t *)device_ptr;
    if (device) {
      const char *device_name = *(const char **)(device + 0x10);
      if (strcmp(device_name, name) == 0) {
        return device_ptr - (uintptr_t)this;
      }
    }
    device_ptr += 8;
  } while (device_ptr < device_list_end);
  return NULL;
}

uint32_t djplayer_uiplayer::getDeckBPM()
{
  // This was actually quite tough to find, the BPM is hidden inside the
  // the djplay::DeviceComponent object which is in a massive list of other
  // device component objects. A device component appears to be any feature
  // on a deck that is polled by rekordbox, such as the bpm control, all the
  // knobs, and buttons.  The device initialization can be found via the
  // string "@BPM" inside djplay::UiPlayer::createDevice().
  void *bpmDevice = nullptr;
  void *bpmDeviceInner = nullptr;
  uint32_t bpm = 0;
  switch (config.version) {
  case RBVER_585:
    bpmDevice = *(void **)((uintptr_t)this + 0xA90);
    bpmDeviceInner = *(void **)((uintptr_t)bpmDevice + 0x80);
    bpm = *(uint32_t *)((uintptr_t)bpmDeviceInner + 0x154);
    break;
  case RBVER_650:
  case RBVER_651:
  case RBVER_652:
  case RBVER_653:
  default:
    // dynamically locate the bpm device for all versions after 650
    if (config.version >= RBVER_650) {
      static uintptr_t bpm_device_offset = 0;
      if (!bpm_device_offset) {
        // this will iterate the list of devices and look for the device with
        // the name @BPM instead of using a hardcoded offset which seems to
        // change anytime new devices are added -- which seems not uncommon
        bpm_device_offset = find_device_offset("@BPM");
      }
      if (!bpm_device_offset) {
        break;
      }
      bpmDevice = *(void **)((uintptr_t)this + bpm_device_offset);
      if (!bpmDevice) {
        break;
      }
      // then the offsets for the bpm within the device should never change
      bpmDeviceInner = *(void **)((uintptr_t)bpmDevice + 0x80);
      if (!bpmDeviceInner) {
        break;
      }
      bpm = *(uint32_t *)((uintptr_t)bpmDeviceInner + 0x154);
      break;
    }
    error("Unknown version");
    break;
  }
  return bpm;
}

uint32_t djplayer_uiplayer::getDeckTime()
{
  // This was actually quite tough to find, the BPM is hidden inside the
  // the djplay::DeviceComponent object which is in a massive list of other
  // device component objects. A device component appears to be any feature
  // on a deck that is polled by rekordbox, such as the bpm control, all the
  // knobs, and buttons.  The device initialization can be found via the
  // string "@BPM" inside djplay::UiPlayer::createDevice().
  void *timeDevice = nullptr;
  void *timeDeviceInner = nullptr;
  uint32_t time = 0;
  switch (config.version) {
  case RBVER_585:
    // no
    return 0;
  default:
    // dynamically locate the bpm device for all versions after 650
    if (config.version >= RBVER_650) {
      static uintptr_t time_device_offset = 0;
      if (!time_device_offset) {
        // this will iterate the list of devices and look for the device with
        // the name @BPM instead of using a hardcoded offset which seems to
        // change anytime new devices are added -- which seems not uncommon
        time_device_offset = find_device_offset("@CurrentTime");
      }
      timeDevice = *(void **)((uintptr_t)this + time_device_offset);
      // then the offsets for the time within the device should never change
      timeDeviceInner = *(void **)((uintptr_t)timeDevice + 0x80);
      time = *(uint32_t *)((uintptr_t)timeDeviceInner + 0x154);
      break;
    }
    error("Unknown version");
    break;
  }
  return time;
}

uint32_t djplayer_uiplayer::getTotalTime()
{
  // This was actually quite tough to find, the BPM is hidden inside the
  // the djplay::DeviceComponent object which is in a massive list of other
  // device component objects. A device component appears to be any feature
  // on a deck that is polled by rekordbox, such as the bpm control, all the
  // knobs, and buttons.  The device initialization can be found via the
  // string "@BPM" inside djplay::UiPlayer::createDevice().
  void *timeDevice = nullptr;
  void *timeDeviceInner = nullptr;
  uint32_t time = 0;
  switch (config.version) {
  case RBVER_585:
    // no
    return 0;
  default:
    // dynamically locate the bpm device for all versions after 650
    if (config.version >= RBVER_650) {
      static uintptr_t total_time_device_offset = 0;
      if (!total_time_device_offset) {
        // this will iterate the list of devices and look for the device with
        // the name @BPM instead of using a hardcoded offset which seems to
        // change anytime new devices are added -- which seems not uncommon
        total_time_device_offset = find_device_offset("@TotalTime");
      }
      timeDevice = *(void **)((uintptr_t)this + total_time_device_offset);
      // then the offsets for the time within the device should never change
      timeDeviceInner = *(void **)((uintptr_t)timeDevice + 0x80);
      time = *(uint32_t *)((uintptr_t)timeDeviceInner + 0x154);
      break;
    }
    error("Unknown version");
    break;
  }
  return time;
}


static djplayer_uiplayer **get_player_list()
{
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
  // To find the offset for the UIManager look for string "Layout" (capital L) and
  // the first occurrence should be shortly before a usage of MainComponent::getInstance()
  // the call right after utilizes the UIManager to call UIManager->InitSkin:
  //
  //    v209 = MainComponent::getInstance();
  //    djplay::UiManager::initSkin(*(v209 + 0x648), &v557, 0);
  //                                         ^ This is the UIManager offset
  //
  // Then finally to get the pPlayers offset look for "WindowID" and at the
  // first occurrence, the bottom of the function looks like this:
  //
  //      djplay::UiManager::createObject(v8);
  //      *(_QWORD *)(v1 + 1608) = v8;
  //    return sub_1049A00((unsigned __int64)&v17 ^ v26);
  //
  // At the start of djplay::UiManager::createObject() is assignments like this:
  //
  //    v4 = (__int64 *)(v1 + 0x50);
  //                          ^ the first UIPlayer offset
  //
  // This is the first UiPlayer of four UiPlayers. It's not an array, they are
  // just consecutive members but they can be accessed like an array.
  static uintptr_t main_component = 0;
  static uintptr_t ui_manager = 0;
  static djplayer_uiplayer **pPlayers = nullptr;
  if (!pPlayers) {
    switch (config.version) {
    case RBVER_585:
      main_component = *(uintptr_t *)(rb_base() + 0x39D05D0);
      break;
    case RBVER_650:
      main_component = *(uintptr_t *)(rb_base() + 0x3F38108);
      break;
    case RBVER_651:
      main_component = *(uintptr_t *)(rb_base() + 0x3F649E8);
      break;
    case RBVER_652:
      main_component = *(uintptr_t *)(rb_base() + 0x3F8B318);
      break;
    case RBVER_653:
      main_component = *(uintptr_t *)(rb_base() + 0x3E4B308);
      break;
    default: // RBVER_661+
        // just going to set this going forward... hopefully it doesn't change
      if (config.version >= RBVER_661) {
        // this sig will dump us 0x1D bytes before a reference to gMainComponent
        uintptr_t main_component_ref = sig_scan(MAIN_COMPONENT_SIG);
        if (!main_component_ref) {
          error("Failed to locate main component reference sig");
          return NULL;
        }
        // this is the offset to the EIP for gMainComponent
        int32_t offset = *(int32_t *)(main_component_ref + 0x1D);
        // the main component is offset bytes from the opcode after the reference
        main_component = *(uintptr_t *)(main_component_ref + 0x21 + offset);
        break;
      }
      error("Unknown version");
      return NULL;
    }
    if (!main_component) {
      error("Failed to locate main component");
      return NULL;
    }
    // version 585 the uimanager is at 0x638, every version after is 0x648
    if (config.version == RBVER_585) {
      ui_manager = *(uintptr_t *)(main_component + 0x638);
    } else {
      ui_manager = *(uintptr_t *)(main_component + 0x648);
    }
    if (!ui_manager) {
      error("UIManager is NULL");
      return NULL;
    }
    // players are always at this offset in the uimanager
    pPlayers = (djplayer_uiplayer **)(ui_manager + 0x50);
    if (!pPlayers) {
      error("Players are NULL");
      return NULL;
    }
    // log the three pieces of info we found
    info("MainComponent: %p", main_component);
    info("UIManager: %p", ui_manager);
    info("Players: %p", pPlayers);
  }
  // pPlayers is really just the first of four consecutive UIPlayers in
  // the UIManager object so we can access them like an array
  return pPlayers;
}

// lookup a djplayer/deck by index (0 to 3)
djplayer_uiplayer *lookup_player(uint32_t deck_idx)
{
  // there's only 4 decks in this object
  if (deck_idx >= 3) {
    deck_idx = 0;
  }
  // pPlayers is really just the first of four consecutive UIPlayers in
  // the UIManager object so we can access them like an array
  djplayer_uiplayer **pPlayers = get_player_list();
  return pPlayers[deck_idx];
}

uint32_t lookup_player_idx(djplayer_uiplayer *player)
{
  if (!player) {
    return 0;
  }
  // pPlayers is really just the first of four consecutive UIPlayers in
  // the UIManager object so we can access them like an array
  djplayer_uiplayer **pPlayers = get_player_list();
  for (uint32_t i = 0; i < 4; ++i) {
    if (pPlayers[i] == player) {
      return i;
    }
  }
  return 0;
}

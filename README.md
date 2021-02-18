# Rekord Box Song Exporter
A hack for Rekordbox 6.5.0 or 5.8.5 on Windows 64bit only.  
Tested on Win10 and Win8.1

![Alt text](/launcher.png?raw=true "Launcher")

## What does it do

This will export played tracks for integration with OBS or any system that can
consume the track names from a file.

This does **NOT** poll the Rekordbox database, this directly hooks Rekordbox which 
means the output files update instantaneously when you fade into another track.

The module offers a variety of configuration options to control how it outputs
the track information along with other unique options.

## How does it work

The Launcher will optionally launch Rekordbox and inject a module which hooks two 
functions, one function is called when the play/cue button is pressed on a deck, and 
the other function is called anytime the 'master' deck switches in Rekordbox.

The play/cue hook will cache each song that is played on any deck into an internal
storage of the hack itself.

The other hook will detect anytime the 'master' switches to another deck, when
this happens it will log the cached track and artist for that deck.

So the expected flow is to load a track, play/cue it, then eventually fade into 
the track. When you fade from one deck into the other Rekordbox will update the 
'master', this will trigger the hack to log the new track title and artist.

There is also edge-case support for when you load a new track onto the master deck while one is already playing there.
F for your terrible transition, but rest assured the song will still be logged correctly.

This works with either two or four decks.

## How to use it

Ensure the Loader and Module are both in the same folder.

You can run Rekordbox beforehand or just use the launcher to start Rekordbox for you. 

You may close the launcher once the module has been injected into Rekordbox.

Once the hack is loaded four output files will appear in the folder of the Loader
and Module, these files will be dynamically updated in different ways with different
information.

### current_track.txt
This file contains one line with the current track playing, nothing else.

### last_track.txt
This file is also one line with only the last track to play, nothing else.

### current_tracks.txt
This contains a rotating list of tracks capped to a maximum number of lines. 
The newest track is at the top and oldest is at the bottom.
The number of lines is configurable in the launcher.

### played_tracks.txt
This is a full log of all tracks played for the entire session.
The oldest track will be at the top and newest at bottom. 
This file can include (hh:mm:ss) timestamp prefixes if 'Timestamps' is enabled.

#### NOTE: All output files are wiped when the module is injected

So if you want to save your played_tracks.txt file inbetween sessions then be sure to make a copy before launching the hack again.

## Configurations

The UI offers configurations for:

### Version
The version of Rekordbox being launched/hooked (only 6.5.0 and 5.8.5 at the moment)

### Specific Path
The path of Rekordbox can be overridden to any location, the version dropdown is still important because the module works differently for each version.
The path is not important if you are running Rekordbox before pressing the Launch button.

### Output Format
The format of output lines for all four files, this doesn't include the timestamp prepended for played_tracks.txt lines.

The available placeholders in the output format include (so far):
```
   %artist%     The artist of the track
   %track%      The track title
   %time%       The current timestamp (hh:mm:ss)
```

### Cur Tracks Count
The number of lines in the current_tracks.txt file, ths file will always be truncated to this number of lines and the track list will be rotated through it.

This is useful for OBS 'chatlog' mode with GDI text object because OBS requires a 'lines' count and will not read the file if the line count goes beyond the limit.

Setting this to for example 10 would allow you to list the 'last 10 tracks' played at any given time.

### Timestamps
This toggles a built-in timestamp on each line of played_tracks.txt, this timer starts when the first song is played and doesn't ever stop.

## OBS Integration

The trick to integrating with OBS is to create a Text GDI object and select the 'read from file' option.

Point the text GDI object at any of the four files and turn on 'chatlog' mode to ensure the object is refreshed anytime the file changes content.

You will need to set a 'lines' count in OBS, for displays such as 'current song' you would want to set this to 1.

If you wanted to have a list of the 'last 15 songs' which continuosly rotates then you would set the launcher config for 'Cur Tracks Count' to 15, and similarly set the OBS 'lines' count in the GDI text object to 15. Then you would point the file at current_tracks.txt

By using GDI text objects in OBS your track listings will update in realtime in about ~2 seconds, the 2 second delay is from OBS polling the chatlog file, not because this hack is delayed in writing the output.

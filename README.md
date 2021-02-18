# Rekord Box Song Exporter
A hack for Rekordbox 6.5.0 or 5.8.5 on Windows 64bit only.  
Tested on Win10 and Win8.1

![Alt text](/launcher.png?raw=true "Launcher")

This will export played tracks for integration with OBS or any system that can
consume the track names from a file.

This does **NOT** poll the rekordbox database, this directly hooks rekordbox which 
means the output files update instantaneously when you fade into another track.

The Launcher will launch RekordBox and inject a module which hooks two functions,
one function is called when the play/cue button is pressed on a deck, and the other 
function is called anytime the 'master' deck switches in Rekordbox.

The play/cue hook will cache each song that is played on any deck into an internal
storage of the hack itself.

The other hook will detect anytime the 'master' switches to another deck, when
this happens it will log the cached track and artist for that deck.

So the expected flow is to load a track, play/cue it, then eventually fade into 
the track. When you fade from one deck into the other Rekordbox will update the 
'master', this will trigger the hack to log the new track title and artist.

There is also edge-case support for when you load a new track onto the master deck.
This would probably sound horrible, but the song will still be logged correctly.

This works with either two or four decks.

There are four output files:

```
   played_tracks.txt - this is a full log of all songs played in the entire session,
                       the oldest song will be at the top and newest at bottom.
                       This file can optionally include (hh:mm:ss) timestamps if
                       the Timestamps checkbox is ticked in the Launcher.
                       
  current_tracks.txt - this is the last 3 songs (configurable: 'Cur tracks count'), 
                       newest song at top and older songs under it
                       
   current_track.txt - this is only the current track playing, nothing else.
   
      last_track.txt - this is only the last track to play, nothing else.
```                    

Each of these output files will be cleared when you first inject the hack into
Rekordbox, so if you want to save your played_tracks.txt file inbetween sessions
then be sure to make a copy before launching the hack again.

The UI offers configurations for:

   - the version of rekordbox being launched/hooked (only 6.5.0 and 5.8.5 at the moment)
   - the path of rekordbox can be overridden to any location, the version dropdown is still important
   - the format of output lines for all four files, this doesn't include the timestamp prepended for played_tracks.txt lines
   - the number of lines in the current_tracks.txt file, ths file will always be truncated to this number of lines
   - whether to include an (hh:mm:ss) timestamp on each line of played_tracks.txt, this timer starts when
     the first song is played and doesn't ever stop.

The trick to integrating with OBS is to create a Text GDI object and select the 
'read from file' option.

Point the text GDI object at any of the four files and turn on 'chatlog' mode to
ensure the object is refreshed anytime the file changes content.

By using GDI text objects in OBS your track listings will update in realtime in about 
~2 seconds, the 2 second delay is from OBS polling the chatlog file, not because this 
hack is delayed in writing the output.

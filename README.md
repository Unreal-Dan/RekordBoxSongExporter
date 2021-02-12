# RekordBoxSongExporter
A hack for Rekordbox 6.5.0 (Windows 64bit ONLY) to export played tracks for integration with OBS

This does **NOT** poll the rekordbox database, this directly hooks rekordbox which 
means your track listings will update in realtime in about ~2 seconds.

The 2 second delay is a result of OBS polling the chatlog file, the file is updated
instantaneously by the Song Exporter module as soon as you fade into the track.

The Launcher will launch RekordBox and inject a module which hooks two functions,
one function is called anytime the play/cue button is pressed on a track, and
the other function is called anytime the 'master' deck switches in Rekordbox.

The hook will detect anytime one of the two decks has a different song loaded
since the last time the user hit play, it will log that song to played_tracks.

The other hook will detect anytime the 'master' switches to another deck, when
this happens it will log the last song that was played to 'current_tracks'.

So the expected flow is to load a track, play/cue it, then eventually fade into 
the track. When you fade from one deck into the other Rekordbox will update the 
'master', this will trigger the hack to log the new track to 'current_tracks'.

The played_tracks file is simply a track log which will append each song you play
and will never be cleared.

The other file contains the last 2 songs played in reverse order, where the newest
song is always at the top of the file and older songs are pushed down the file to
a maximum of 2 lines. The line count can be configured if you build from source.

The second file is the trick to integrating with OBS, create a GDI text object in 
OBS and point it at that file then enable chatlog mode and adjust the max lines 
to 2 (or however many you want if you built from source).

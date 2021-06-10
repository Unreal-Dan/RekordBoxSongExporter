# Rekord Box Song Exporter
A hack for Rekordbox 6.5.1, 6.5.0 or 5.8.5 on Windows 64bit only.  
Tested on Win10 and Win8.1

### You can find prebuilt releases [here](https://github.com/Unreal-Dan/RekordBoxSongExporter/releases)

<img src="/launcher_new.png" />

## What does it do

This will export played tracks in realtime for integration with OBS or any system 
that can consume the track names from a file.

This does **NOT** poll the Rekordbox database, this directly hooks Rekordbox which 
means there is no added risk of database corruption, and no delay when the output 
files update.

The module offers a variety of configuration options to control how it outputs
the track information along with other unique options.

The optional server allows you to send the track information to another PC and
perform the logging on that pc instead. For situations where Rekordbox may be running
on a separate PC from OBS.

## Getting Started

If you run Rekordbox and OBS on different PCs then you must start the Server on the 
streaming pc first, otherwise ignore the Server.

Place the RekordboxSongExporter launcher and module in a folder somewhere like your
desktop, do not place the Launcher in the Program Files directory of Rekordbox because
Windows will block it from creating files there. 

If you just want to get up and running then the default configuration should be good
right out of the box, you shouldn't need to touch anything except the version dropdown
to ensure the software is interfacing with the correct version of Rekordbox.

Use the Launcher to start Rekordbox and a folder called OutputFiles should appear in
the same directory as the launcher.

If you have any issues generating the OutputFiles folder then try moving the Launcher 
and Module to a different location and retrying, it's possible Windows 10 may be blocking
the creation of files in the chosen location.

Open OBS and create a GDI text object then enable 'Read From File' and point it at one 
of the files that were created inside the OutputFiles directory. These filenames should
be exactly the same as the list of Output Files that were configured in the launcher.

Turn on chatlog mode on the GDI text object in OBS and you're good to go.

## Understanding the Launcher

<p align="center">
  <img src="/launcher_explained.png" />
</p>

## How does it work

The Launcher will optionally launch Rekordbox and inject a module which hooks two 
of Rekordbox's functions: one function is called when the play/cue button is pressed
on a deck, and the other function is called when the 'master' deck switches.

The play/cue hook will cache each song that is played on any deck into an internal
storage of the hack itself.

The other hook will detect anytime the 'master' switches to another deck, when
this happens it will log the cached track and artist for that deck.

So the expected flow is to load a track, play/cue it, then eventually fade into 
the track. When you fade from one deck into the other Rekordbox will update the 
'master', this will trigger the hack to log the new track title and artist.

There is also edge-case support for when you load a new track onto the master deck 
while a track is already playing there. F in chat for your terrible transition, but 
rest assured the song will still be logged correctly.  

This works with either two or four decks.

## Detailed Usage Instructions

 1. Ensure the Loader and Module are both in the same folder as each other in a user writable location (not Program Files or C:/)

    1a. If you're running Rekordbox and OBS on the same PC you can ignore the Server.

    1b. If you are running Rekordbox and OBS on separate PCs then you need to run the 
        the Server on the streaming PC first, then enter the stream PC IP into Launcher.

 2. You can run Rekordbox beforehand or just use the launcher to start Rekordbox for you. 

 3. You may close the launcher once the module has been injected into Rekordbox.

 4. Once the hack is loaded a folder called OutputFiles will appear.
 
    4a. If you're running locally the folder will appear in the same location as as the launcher/module
    
    4b. If you're running the Server then the folder will appear in the same locaiton as the Server
 
 5. Create a GDI text object in OBS and point it at one of the output files
 
 6. Make sure to enable chatlog mode on the GDI text object

All of the output files will dynamically update with the latest track info based on the configuration

#### NOTE: All output files are wiped when the module is injected

If you want to save your TrackList file inbetween sessions then be sure 
to make a copy before launching the hack again.

### Output Format Placeholders

The format can be enriched with various tags which are replaced with the track information in realtime.

The available placeholders in the output format include (so far):
```
    %title%           The title
    %artist%          The artist
    %album%           The album
    %genre%           The genre
    %label%           The label
    %key%             The key
    %orig_artist%     The original artist
    %remixer%         The remixer
    %composer         The composer
    %comment%         The comment
    %mix_name%        The mix name
    %lyricist%        The lyricist
    %date_created%    The date created
    %date_added%      The date added
    %track_number%    The track number
    %bpm%             The original track bpm (not deck bpm)
    %time%            The current timestamp (hh:mm:ss)
```

An example format might be: ```%artist% - %title% (bpm: %bpm%)```

## Server Mode

If you are running OBS on the same PC as Rekordbox then you don't need the server,
you can just run the Launcher and leave the server checkbox unchecked.

The server will listen for connections from the module on port ```22345``` (TCP). 

Standard port forwarding rules apply for connections outside of your local network, 
the connection is not encrypted and this is intended for LAN use.

The server should utilize all the same configuration options that are setup in the
Launcher.

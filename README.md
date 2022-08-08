# Rekord Box Song Exporter
A hack for Rekordbox on Windows 64bit only.  
Officially Supported Rekordbox Versions: 6.6.4,  6.6.3,  6.6.2,  6.6.1,  6.5.3,  6.5.2,  6.5.1,  6.5.0,  5.8.5  
Tested on Win10 and Win8.1

### You can find prebuilt releases [here](https://github.com/Unreal-Dan/RekordBoxSongExporter/releases)

The latest version of RBSE (6.7+) only works on versions 6.6.4+
If you want to use an older verison of rekordbox then simply use a version of RBSE before 3.7

<img src="/launcher_new.png" />

## What does it do

This will export track information in realtime for integration with OBS or any system
that can consume the information from a file.

This does **NOT** poll the Rekordbox database, this directly hooks Rekordbox which
means there is no added risk of database corruption, and no delay when the output
files update.

The module offers a variety of configuration options to control how it outputs the
track information along with other unique options.

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
of the files that were created inside the OutputFiles directory. These filenames correspond
to the Output Files list in the launcher.

Turn on chatlog mode on the GDI text object in OBS and you're good to go.

## Understanding the Launcher

<p align="center">
  <img src="/launcher_explained.png" />
</p>

## Output File Configurations

The loader will come with a series of output files already configured, but you may want to modify these output files or configure your own.

The default output files that come with the software are:

```
    TrackTitle          This is the current playing track's Title
    TrackArtist         This is the current playing track's Artist
    LastTrackTitle      This is the Title of the last track to play
    LastTrackArtist     This is the Artist of the last track to play
    TrackList           This is a timestamped list of all tracks
```

When configuring an output file (green section) the following options are available:

### Filename

This field is simply the name of the file, name it anything you like but it is advised to give it a name that reflects the content. For example: CurTrackTitle or LastTrackArtist

### Line Format

This specifies exactly how each line will be formatted in the output file, you can utilize various placeholders which are replaced with the track information in realtime.

The available placeholders in the output format include (so far):
```
    %title%             The title
    %artist%            The artist
    %album%             The album
    %genre%             The genre
    %label%             The label
    %key%               The key
    %orig_artist%       The original artist
    %remixer%           The remixer
    %composer           The composer
    %comment%           The comment
    %mix_name%          The mix name
    %lyricist%          The lyricist
    %date_created%      The date created
    %date_added%        The date added
    %track_number%      The track number
    %bpm%               The original track bpm (not deck bpm)
    %time%              The current timestamp (hh:mm:ss)

    %deck1_bpm%         The BPM of deck 1 at the time the track changes
    %deck2_bpm%         The BPM of deck 2 at the time the track changes
    %deck3_bpm%         The BPM of deck 3 at the time the track changes
    %deck4_bpm%         The BPM of deck 4 at the time the track changes
    %master_bpm%        The master BPM at the time the track changes

    %rt_deck1_bpm%      The realtime BPM of deck 1, this will trigger updates
    %rt_deck2_bpm%      The realtime BPM of deck 1, this will trigger updates
    %rt_deck3_bpm%      The realtime BPM of deck 1, this will trigger updates
    %rt_deck4_bpm%      The realtime BPM of deck 1, this will trigger updates
    %rt_master_bpm%     The realtime master BPM, this will trigger updates
```

For example, to create an output file with the current track title the format would be: ```%track%```.

A full track list with timestamps, artist, and title would look something like this: ```%time% %artist% - %track%```

You can use the BPM tags such as ```%deck1_bpm``` and ```%master_bpm%```, these tags will log the BPM of the respective
deck at the time of the track changing.

There is also 'realtime BPM tags', these tags will trigger updates to the respective output file upon detecting a change
to the given BPM. For example ```%rt_deck1_bpm%``` will trigger an update anytime the tempo is adjusted on deck 1.

The reason for the separate BPM tags is for example if you want to create a full track list with timestamps that also 
includes BPM information then you will need to use the non-realtime BPM tags otherwise you will end up with duplicate 
tracks in the tracklist each time the BPM changes, ex: ```%time% %artist% - %title% (BPM: %master_bpm%)```

However, if you want to create a realtime BPM display which updates whenever the BPM changes then you will need to
use the realtime BPM tags, for example: ```Current BPM: %rt_master_bpm%```

### Output Mode

This controls how lines are written to the output file, the options are ```Replace```, ```Append```, ```Prepend```.

In ```Replace``` mode the file is entirely wiped each time a new line is written, the file never exceeds 1 line.

In ```Append``` mode the line is added to the bottom of the file, all previous contents retained above the new line.

In ```Prepend``` mode the line is inserted at the top of the file, all previous contents retained below the new line.

### Offset

The ```Offset``` controls how many tracks must play before the output file will start being updated.

For example ```LastTrackArtist``` has an offset of 1, and is set to ```Replace``` mode with a format of ```%artist%```.

### Max Lines

The ```Max Lines``` controls how large the output file can grow.

This control is only applicable to ```Prepend``` mode because in ```Replace``` mode the file will never get bigger than 1 line, and
in ```Append``` mode it doesn't make sense to add new lines to the bottom and then immediately trim them off.

The main purpose of the Max Lines control is to create a Rotating Track List with Prepend Mode. Unfortunately OBS won't read output
files if the number of lines goes beyond the configured line count in OBS's GDI text object.

A good way to think about the ```Offset``` and ```Max Lines``` is like a ```start``` and ```end``` of the last X tracks to play.

For example if the offset is 3 and max lines is 5 then you will get the 3rd to 8th tracks.

Setting Max Lines to 0 indicates the file has no line limit.

It is suggested you review the example output files that come with the Launcher to familiarise yourself with how Max Lines and Offset can be used.

## Detailed Usage Instructions

 1. Ensure the Loader and Module are both in the same folder as each other in a user writable location (not ProgramFiles or C:/)

    1a. If you're running Rekordbox and OBS on the same PC you can ignore the Server.

    1b. If you are running Rekordbox and OBS on separate PCs then you need to run the the Server on the streaming PC first, then enter the stream PC IP into Launcher.

 2. You can run Rekordbox beforehand or just use the launcher to start Rekordbox for you.

 3. You may close the launcher once the module has been injected into Rekordbox.

 4. Once the hack is loaded a folder called OutputFiles will appear.
 
    4a. If you're running locally the folder will appear in the same location as as the launcher/module
   
    4b. If you're running the Server then the folder will appear in the same location as the Server
 
 5. Create a GDI text object in OBS and point it at one of the output files
 
 6. Make sure to enable chatlog mode on the GDI text object

All of the output files will dynamically update with the latest track info based on the configuration

#### NOTE: All output files are wiped when the module is injected

If you want to save your TrackList file in between sessions then be sure
to make a copy before launching the hack again.

## Server Mode

If you are running OBS on the same PC as Rekordbox then you don't need the server,
you can just run the Launcher and leave the server checkbox unchecked.

The server will listen for connections from the module on port ```22345``` (TCP).

Standard port forwarding rules apply for connections outside of your local network,
the connection is not encrypted and this is intended for LAN use.

The server should utilize all the same configuration options that are setup in the
Launcher.

## How does it work

The Launcher will optionally launch Rekordbox and inject a module which hooks two 
of Rekordbox's functions: one function is called when a track is loaded onto a deck,
and the other function is called when the 'master' deck switches.

The track loading hook will cache each song that is played on any deck into an 
internal storage of the hack itself.

The other hook will detect anytime the 'master' switches to another deck, when
this happens it will log the cached track and artist for that deck based on the 
output file configurations.

So the expected flow is to load a track, play/cue it, then eventually transition 
into the track. When you switch from one deck into the other Rekordbox will update 
the 'master', this will trigger the hack to log the new track title and artist.

There is also edge-case support for when you load a new track onto the master deck
while a track is already playing there. F in chat for your terrible transition, but
rest assured the song will still be logged correctly.  

This works with either two or four decks.

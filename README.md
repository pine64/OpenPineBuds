# PineBuds Open Source SDK

This is just the SDK from the wiki (so far), with dockerfile setup to make building easier, and my reverse engineered bestool integrated.
The docker image will build bestool for you first, then drop you into the dev container for building and flashing the buds.

## Usage

To use this setup to build & flash your pine buds you will need a system with docker setup at the minimum.
Docker is used to (1) make this all much more reprodicible and easier to debug and (2) so that we dont mess with your host system at all.
In order to program the buds from inside of the docker container; privileged mode is used. So do be a tad more careful than usual.

```bash

./start_dev.sh # This will cause docker to start your working environment; this should take roughly 1-3 minutes depending on your network speed to the GCC hosting server

# Now you will be inside of the container, and your prompt will look akin to "root@ec5410d0a265:/usr/src#"

./build.sh # This will run make and build the output program. If you have weird build errors try running clean.sh or rm -rf'ing the out folder first

# Now that the firmware has finished building; if there are no errors you can load it to your buds

# You may want to back up the firmware currently on the buds - it will be deleted when the new firmware is loaded on:
./backup.sh

# You may need to take the buds out of the case, wait three seconds, place them back. This wakes them up and the programmer needs to catch this reboot.

# You can try the helper script by running
./download.sh

# Or do it manually by :

# Assuming that your serial ports are 0 and 1, run the following to commands to program each bud in series.
bestool write-image out/open_source/open_source.bin --port /dev/ttyACM0
bestool write-image out/open_source/open_source.bin --port /dev/ttyACM1
```

## Changelist from stock opeen source SDK

- Long hold (5 ish seconds) the button on the back when buds are in the case to force a device reboot (so it can be programmed)
- Use the resistor in the buds to pick Left/Right rather than TWS master/slave pairing
- Pressing the button on the back while in the case no longer triggers DFU mode
- Debugging baud rate raised to 2000000 to match stock firmware
- Fixed TWS operation such that putting either bud into the case correctly switches to the other bud
- Working (mostly) audio controls using the touch button on the buds
- Turned off showing up as a HID keyboard, as not sure _why_ you would; but it stops android nagging me about a new keyboard

## Current bud tap codes

### Both pods active

#### Right Ear:

- Single tap : Play/Pause
- Double tap : Next track
- Hold : Toggle ANC(Currently non functional, WIP)
- Triple tap : Volume Up

#### Left Ear:

- Single tap : Play/Pause
- Double tap : Previous track
- Hold : Toggle ANC(Currently non functional, WIP)
- Triple tap : Volume Down

### Single pod active

- Single tap : Play/Pause
- Double tap : Next track
- Hold : Previous track
- Triple tap : Volume Up
- Quad tap : Volume Down

## Changing audio alerts
The audio alerts are stored in: 

`config/_default_cfg_src_/res/en/`

You can convert these .txt files to .wav files by using the script `convert.sh` with the -T flag, the resulting file will be named `output.wav`.
If you want to change the alert to a custom sound use the -W flag on your wav file and then move the resulting `SOUND.txt` file to the place you want to replace (eg. `config/_default_cfg_src_/res/en/SOUND_POWER_ON.txt`)

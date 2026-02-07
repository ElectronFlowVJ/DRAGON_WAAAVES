# DRAGON_WAAAVES v1.0

If you like the work I've done here, please consider supporting me on Ko-fi @ ko-fi.com/electronflow. 

Please also consider supporting Andrei Jay either on Patreon, or his Ko-fi @ ko-fi.com/andreijay as this would not have been possible without the amazing groundwork he has laid out, and his willingness to open source his projects.

**Andrei's website:** videosynthecosphere.com

You can download WPDSK here, which is a good introduction to the concepts present in this application, you can also buy GW-DSK there, which you should do. PLEASE GO SUPPORT HIM.

**Andrei's other website:** andreijaycreativecoding.com

A wealth of knowledge and resources about video synthesis, feedback, and programming these sorts of things.

**ElectronFlow's Website:** electronflow.tv

my art mostly

## Disclaimer

**This is NOT an official port of Gravity Waaaves.** The original creator, Andrei Jay, will not provide any support whatsoever for this software. 

This software is distributed AS-IS, there is no guarenteed continuing support. Please contact ElectronFlow on discord with any questions, make sure you let me know what it's about.

## Distribution

This software is only to be distributed as source code or on a Jetson disk image. Do not redistribute compiled binaries.

---

## What It Is

A real-time video effects processor and synthesizer built with openFrameworks. Designed for live performance, VJing, and media installations.

## Key Features

- 3-block video processing chain with dual feedback loops
- Multiple input sources: webcam, NDI, Spout (Windows)
- OSC control with 700+ addressable parameters for automation
- MIDI macro mapping (16 macros per parameter group)
- Streaming output via NDI and Spout
- Preset bank system with save/load functionality
- Built in Lassjous Shape Generator

## Requirements

- NewTek NDI: ndi.video
- Spout2: spout.zeal.co

- openFrameworks 0.12+
- Required addons (see `addons.make`):
  - ofxImGui (develop branch)
  - ofxMidi
  - ofxNDI - https://github.com/leadedge/ofxNDI
  - ofxOsc
  - ofxSpout - https://github.com/leadedge/Spout2 (Windows only) this will likely give you issues adding to the openFrameworks project using Project Generator, just download and add to the addOns folder, everything else should already be in place.


## Usage Tips

- The application runs with two windows: a control window (GUI) and an output window, but can also be used on single display, not recommended though.
- FPS is configurable from 1-60 (default 30)
- The output window can be set to fullscreen for production using F11, deocrations can be toggled with F10
- Presets are stored in `bin/data/presets/`

## OSC Reference

See `bin/data/OSC_Parameters_Reference.txt` for complete OSC address documentation.

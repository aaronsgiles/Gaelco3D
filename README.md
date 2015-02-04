# Gaelco3D
Standalone emulator for early Gaelco 3D games

Enclosed is the source code for my standalone Gaelco 3D game emulator. It is kind of a quick and dirty implementation, but the key aspects are static high level emulation of the TMS32031 and ADSP2100.

Previously, only the Radikal Bikers emulator has been released. However, I had done some work on Surf Planet and Speed Up as well.

The current status of each game is as follows:

* Radikal Bikers: ADSP2100 and TMS32031 are both emulated with C code for maximum speed. As I recall there were some occasional glitches present, but overall it runs pretty well with full sound.

* Surf Planet and Speed Up: both the ADSP2100 and TMS32031 emulations are copied from Radikal Bikers and don't work. They would need to be reimplemented based on analyzing the game-specific code. This means sound doesn't work in either game. For the TMS32031, the C code has been disabled with a fallback to running the TMS32031 through a slow interpreter. Thus, the games boot up and run, but not particularly fast. No effort has been made to map the controls.

When run full-screen, all games render their graphics at the full screen resolution. When run in a window, games run at their native resolution. FPS display can be seen in the title bar of the window.

The build environment is Visual Studio command-line. Only 32-bit builds have ever been attempted. There are small two-letter batch files to wrap the nmake commands.

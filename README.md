Gaelco3D
========
Standalone emulator for early Gaelco 3D games.

Enclosed is the source code for my standalone Gaelco 3D game emulator. It is kind of a quick and dirty implementation, but the key aspects are static high level emulation of the TMS32031 and ADSP2100.

Previously, only the Radikal Bikers emulator has been released. However, I had done some work on Surf Planet and Speed Up as well.

Build Tools
===========
The code has been built from the command line using Visual Studio 2008 tools on a Windows machine running Windows 8.1. The two-letter batch files at the root of the project show how to invoke nmake.

Contents
========
The current status of each game is as follows:

* Radikal Bikers: ADSP2100 and TMS32031 are both emulated with C code for maximum speed. As I recall there were some occasional glitches present, but overall it runs pretty well with full sound.

* Surf Planet and Speed Up: both the ADSP2100 and TMS32031 emulations are copied from Radikal Bikers and don't work. They would need to be reimplemented based on analyzing the game-specific code. This means sound doesn't work in either game. For the TMS32031, the C code has been disabled with a fallback to running the TMS32031 through a slow interpreter. Thus, the games boot up and run, but not particularly fast. No effort has been made to map the controls.

When run full-screen, all games render their graphics at the full screen resolution. When run in a window, games run at their native resolution. FPS display can be seen in the title bar of the window.

License
=======
Copyright (c) 2015, Aaron Giles
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Third-Party Notices

## SDL 3 (statically linked into the client libraries)

- Version: 3.4.16 (`third_party/SDL`, tag `release-3.4.16`)
- Source: https://github.com/libsdl-org/SDL
- License: zlib

```
Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
```

SDL 3 bundles further components under compatible licenses. hidapi (`third_party/SDL/src/hidapi`)
is offered under a choice of licenses; this project uses it under the BSD-style license in
`third_party/SDL/src/hidapi/LICENSE-bsd.txt`. See `third_party/SDL/LICENSE.txt` and the license
headers in the SDL source files.

## Half-Life SDK and TWHL Updated projects

- Half Life 1 SDK, Copyright (c) Valve Corp. — [Half Life 1 SDK License](LICENSES/HL1-SDK-LICENSE.txt)
- Half-Life Updated, Opposing Force Updated, Blue Shift Updated — TWHL community, distributed
  under the Half Life 1 SDK License. Exact upstream commits are listed in `games/*/UPSTREAM`.
- The SDK copies in `games/*` include headers and import libraries for libraries that ship
  with Half-Life (`vgui`, `SDL2`). They are used only to build against the game; they are
  not redistributed in the mod package.

```
DISCLAIMER OF WARRANTIES.  THE HALF LIFE 1 SDK AND ANY OTHER MATERIAL
DOWNLOADED BY LICENSEE IS PROVIDED "AS IS".  VALVE AND ITS SUPPLIERS
DISCLAIM ALL WARRANTIES WITH RESPECT TO THE SDK, EITHER EXPRESS OR IMPLIED,
INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY,
NON-INFRINGEMENT, TITLE AND FITNESS FOR A PARTICULAR PURPOSE.

LIMITATION OF LIABILITY.  IN NO EVENT SHALL VALVE OR ITS SUPPLIERS BE LIABLE
FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES WHATSOEVER
(INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS,
BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY
LOSS) ARISING OUT OF THE USE OF OR INABILITY TO USE THE ENGINE AND/OR THE
SDK, EVEN IF VALVE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
```

## Not included

- No Steamworks SDK headers are included. Optional Steam on-screen keyboard support
  calls the game's own `steam_api` library at run time.
- No Source SDK code is included. The VGUI2 integration uses this project's own
  minimal interface declarations.

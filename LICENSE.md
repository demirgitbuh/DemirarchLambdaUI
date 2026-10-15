# Licensing

Demirarch Lambda UI combines original code with code derived from the Half-Life SDK.
They are licensed separately. No single license applies to the whole repository.

| Path | License | Notes |
|---|---|---|
| `core/`, `platform/`, `tools/`, `tests/`, `cmake/`, `docs/` | [MIT](LICENSES/MIT.txt) | Original code that does not use the Half-Life SDK |
| `bridge/` | [Half Life 1 SDK License](LICENSES/HL1-SDK-LICENSE.txt) | Integration code compiled together with SDK code |
| `games/hl`, `games/opfor`, `games/bshift` | [Half Life 1 SDK License](LICENSES/HL1-SDK-LICENSE.txt) | Based on the TWHL Half-Life Updated, Opposing Force Updated and Blue Shift Updated projects (see each `UPSTREAM` file) |
| `gamedir/` | MIT | Package metadata only |
| `third_party/SDL` | zlib | See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) |

The Half Life 1 SDK License allows the modified game to be distributed in source
and object code form **only for free**. Because of that condition, the game libraries
built from this repository are not available under an OSI-approved open source
license, and code licensed under the GPL cannot be combined with them.

This project does not include any Half-Life, Opposing Force or Blue Shift game
content. You need your own legal copies of the games from Steam.

Half-Life, Opposing Force, Blue Shift, Steam and the Half-Life logo are trademarks of
their respective owners. This project is not affiliated with or endorsed by Valve
Corporation or Gearbox Software.

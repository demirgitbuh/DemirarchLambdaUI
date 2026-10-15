# Demirarch Lambda UI

A gamepad-first controller and interface layer for the current Steam versions of
**Half-Life**, **Half-Life: Opposing Force** and **Half-Life: Blue Shift**. It installs as
separate mods and does not overwrite any original game file.

> **Status: in development — not released.** Version 0.1.0 has not been completed.
> The feature list below describes the agreed scope, not what works today. See
> [docs/support-matrix.md](docs/support-matrix.md) for what has actually been verified.

## Scope of 0.1.0

- Full gamepad control from launch to exit: menus, new game, gameplay, weapon wheel,
  saving/loading, settings, text entry, console, multiplayer screens and dialogs.
- Separate mods: `lambda_hl`, `lambda_op4`, `lambda_bs`.
- Platforms: Windows 11, Ubuntu 24.04 LTS, SteamOS (Steam Deck).
- Classic mode: type `hlclassic` in the console to turn off everything this mod adds,
  and `hlgamepad` to turn it back on. The choice is remembered.

## Multiplayer warning

Multiplayer use is not recommended. Compatibility with VAC or other anti-cheat systems
has not been guaranteed. Using client modifications on protected servers may expose your
account to sanctions, including bans. Disabling aim assist does not establish anti-cheat
safety. Proceed only if you understand and accept this risk.

Because Lambda UI is a separate mod, it can only join servers that run the same mod.
It cannot join regular Half-Life servers.

## Building

All builds run on GitHub Actions (`.github/workflows/ci.yml`). To build locally:

```
git clone --recursive https://github.com/demirgitbuh/DemirarchLambdaUI.git
cmake -S DemirarchLambdaUI -B build -A Win32          # Windows (Visual Studio 2022)
cmake --build build --config Release
```

Linux (32-bit, GCC 9 or newer with multilib):

```
cmake -S DemirarchLambdaUI -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/LinuxI386.cmake
cmake --build build
```

The portable package is assembled in `build/stage`.

## License

See [LICENSE.md](LICENSE.md). Game code is under the Half Life 1 SDK License (free
distribution only); original engine-independent code is under the MIT License.

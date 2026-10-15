# Support Matrix

This matrix records what has actually been verified. A feature is only listed as
**Verified** after a physical test on the listed hardware, OS, game build and connection.

## Status legend

| Status | Meaning |
|---|---|
| **Verified** | Physically tested on the listed hardware and configuration. Date and result recorded. |
| **Implemented — awaiting hardware test** | Code exists and automated tests pass, but it has not been tried on real hardware. |
| **Not supported by hardware/API** | The device, connection or platform API does not provide this feature. |
| **Technical blocker** | Cannot be implemented through the target engine or a legal integration path. See notes. |
| **Failed** | Tested and did not work. See notes. |
| **Not implemented yet** | Development in progress. Not part of a finished release. |

## Target software

| Item | Version |
|---|---|
| Half-Life (Steam, appid 70) | Build 15961492 (inspected 2026-10-16, default branch) |
| Opposing Force (appid 50) | Installed with Half-Life build above |
| Blue Shift (appid 130) | Installed with Half-Life build above |
| Windows | Windows 11 (development machine: 10.0.26200) |
| Linux LTS | Ubuntu 24.04 LTS (proposed; no Linux test machine yet) |
| SteamOS | Current stable SteamOS (exact version recorded at test time; no Steam Deck yet) |

## Reference controllers

Exact USB/Bluetooth IDs are recorded only when a device is actually tested.

| Family | Reference model | Connections in scope |
|---|---|---|
| Xbox 360 | Xbox 360 Controller (wired); Xbox 360 Wireless Controller + Xbox 360 Wireless Receiver for Windows | USB; receiver |
| Xbox One | Xbox Wireless Controller, model 1708 | USB, Bluetooth, Xbox Wireless Adapter |
| Xbox Series X\|S | Xbox Wireless Controller, model 1914 | USB, Bluetooth, Xbox Wireless Adapter |
| Xbox Elite | Xbox Elite Wireless Controller Series 2, model 1797 | USB, Bluetooth, Xbox Wireless Adapter |
| Generic XInput | Any controller that presents itself as XInput (tested models listed individually) | USB |
| DualShock 3 | Sony DualShock 3 (CECHZC2) | USB; Bluetooth (needs an extra driver/pairing tool) |
| DualShock 4 | Sony DualShock 4 v2 (CUH-ZCT2) | USB, Bluetooth |
| DualSense | Sony DualSense (CFI-ZCT1) | USB, Bluetooth |
| DualSense Edge | Sony DualSense Edge (CFI-ZCP1) | USB, Bluetooth |
| PlayStation-layout third-party | Candidates: Nacon Revolution 5 Pro (with its USB receiver), Razer Wolverine V2 Pro (PlayStation version) | Per manufacturer |
| Steam Deck | Steam Deck LCD and Steam Deck OLED (built-in controls) | Built-in; through Steam Input |

## Platform and driver constraints (known before testing)

| Constraint | Source |
|---|---|
| Xbox Wireless Adapter on Linux/SteamOS needs the out-of-tree `xone` driver. | medusalix/xone, dlundqvist/xone |
| DualShock 3 on Windows needs a third-party driver (e.g. DsHidMini). | nefarius/DsHidMini |
| DualSense advanced (voice-coil) haptics are delivered as USB audio channels and are **not available over Bluetooth**. | Device behaviour; SDL3 does not expose them |
| XInput does not report Xbox Elite back paddles. Paddles need Windows.Gaming.Input / GameInput / Bluetooth HID paths. | SDL3 source (`SDL_windows_gaming_input.c`) |
| Steam Deck back buttons are mapped through Steam Input; independent direct access is not assumed. | Project specification |

## Results

No physical tests have been run yet. Every row below starts as **Not implemented yet**
and is updated as features are implemented and tested.

| Date | OS | Game | Controller | Connection | Input path | Basic | Gyro | Touchpad | Rumble | Adv. haptics | Adaptive triggers | LED | Battery | Result |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| — | — | — | — | — | — | — | — | — | — | — | — | — | — | No tests yet |

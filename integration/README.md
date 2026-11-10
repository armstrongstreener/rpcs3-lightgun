# MLGB RPCS3 motion and gesture integration

Base: RPCS3 0.0.43-20573, upstream `47599599a4cea37f350eb0936d7c5e4de6f7bf57`.
This handoff contains emulator changes and authored configuration only. No game
executables, extracted game scripts, firmware, SDKs, or gun device IDs are included.
Keep RPCS3's LICENSE and existing source notices with redistributed changes.

## Get the complete source

Repository: https://github.com/armstrongstreener/rpcs3-lightgun
Development branch: `mlgb-lightgun`.

```sh
git clone --recursive --branch mlgb-lightgun https://github.com/armstrongstreener/rpcs3-lightgun.git
```

GitHub's source ZIP contains the source but does not populate Git submodules.
Use the normal RPCS3 build prerequisites and initialize submodules when building.
The optional workspace SDK helpers in this fork are separate from these fixes.

## Apply to the original build

`MLGB-Lightgun.patch` includes all dependent input/profile/UI changes and new
headers. It is checked against the imported source of the upstream base above.
In a clean checkout of that commit:

```sh
git apply --check /path/to/MLGB-Lightgun.patch
git apply /path/to/MLGB-Lightgun.patch
```

For a newer fork, port the indicated functions and configuration fields rather
than replacing its entire cellGem.cpp. The patch intentionally omits build-kit
workflows and development SDK files.

## Motion fix, without gesture actions

Main implementation: `rpcs3/Emu/Cell/Modules/cellGem.cpp`, particularly
`pos_to_gem_state`, `mouse_pos_to_gem_state`, `cellGemGetInertialState`, and the
transient state in `gem_config`.

The original mouse path changed the reported quaternion while leaving angular
velocity/gyro at zero. This fork derives controller-local angular velocity from
`conjugate(previous quaternion) * current quaternion` and guest-time delta, then
rotates it into world angular velocity for CellGemState. It also supplies
orientation-consistent gravity and gyro in CellGemInertialState for The Shoot.
State is independent per controller. Updates are spaced at least 1 ms; first,
rewound and over-100 ms gaps reset the motion estimate. This is a synthetic
mouse-backed stream, not physical IMU fusion.

The compatibility block is gated to Raw Mouse and these tested retail IDs:

- The Shoot USA: `BCUS98222`.
- The House of the Dead: Overkill Extended Cut USA: `BLUS30774`.

Overkill uses normal pose/angular-velocity output; its gesture switches are off.
Other title IDs are not automatically opted in. Equivalent regional IDs require
review and testing before extending the title gates.

The Shoot additionally has a 512-sample transient CellGemState history, sampled
at >=1 ms, for timestamp and negative-current-time queries. Missing old samples
return TIME_OUT_OF_RANGE. This was added during diagnosis; it was not independently
shown to be the decisive aiming fix. Retain and test it if porting the full change.
Historical prediction, inertial sample history and save-state restoration are not
complete implementations.

## The Shoot: three gesture replacements

Helpers: `rpcs3/Input/lightgun_spin.h`. All gesture runtime helpers are gated to
`BCUS98222` plus per-player profile switches in `raw_mouse_config.h`.

| Action | Synthetic data | Current physical binding |
|---|---|---|
| Showtime | 360-degree horizontal yaw and a 600 mm-radius X/Z controller orbit over 0.6 seconds | Right click |
| Point down / floor shot | Hold a -90-degree X-axis pitch; actual trigger supplies the shot | Hold middle click, then trigger |
| Point up / air shot | Off-screen aim prepares +90-degree X-axis pitch; actual trigger supplies the new press | Aim off-screen and trigger |

The Showtime check observed in the retail game requires front/side/back/opposite
side/front in <1.8 seconds and >1500 mm of travel across recorded directions.
Orientation-only rotation failed; the horizontal turn plus positional orbit worked.
The floor/air checks require the corresponding facing direction (dot >0.8) and
a new trigger press. The emulator does not activate game power-ups directly.

Priority is active Showtime, then explicit point-down, then off-screen point-up.
Square and Triangle are consumed as gesture inputs when the corresponding feature
is enabled. The raw profile maps physical inputs to virtual buttons; the supplied
`gem_mouse.yml` routes virtual 3 to Square and virtual 8 to Triangle. Preserve that
routing or adapt the gesture inputs when using an existing fork's bindings.

Switches per player:

- `The Shoot Spin 360: true`
- `The Shoot Point Down: true`
- `The Shoot Point Up Offscreen: true`
- `The Shoot Offscreen Middle: false` (older optional shortcut, currently disabled)

A held Showtime input does not repeat the turn. Releasing and pressing again
re-arms it. Gesture state belongs to each controller independently.

## Profiles and gun assignments

Copy the files under `integration/config` to the emulator's `config` directory.
`raw_mouse.template.yml` is a template: rename it to `raw_mouse.yml` only for a
fresh setup, then assign the recipient's own guns. Do not overwrite an existing
global config without merging it. The game profiles contain no device IDs.

All player Device entries come solely from `config/raw_mouse.yml`. Profile loads
ignore legacy per-game Device entries, and game-profile saves omit them. The UI
shows inherited devices read-only in game profiles; Global defaults edits the
assignments. The supplied gem_mouse.yml contains the virtual PS Move action
routing; merge it if the target already has a customized configuration.

The raw input changes also support primary and alternative keyboard/mouse bindings
with OR-held semantics: releasing one alternative does not release an action that
is still held by the other. Profile load/save lives in `raw_mouse_config.cpp`;
ID stripping is in `raw_mouse_profile.h`. Qt profile editing is in
`raw_mouse_settings_dialog.*`; game-list access is in `game_list_context_menu.cpp`.

Current The Shoot profile: 5 = Move; right = Showtime; middle held = down;
off-screen + trigger = up/air shot. These mappings and gestures were reported
working in-game by the tester. Do not generalize this to untested games.

Current Overkill profile: P1 Start 1 / Select 5; P2 Start 2 / Select 6.
Both guns were reported working when calibrating P2 before P1. Aim tuning is
still in progress: horizontal 90%, vertical 80% are the latest test values,
not a universal calibration. No Overkill gesture actions are enabled.

`Move Aim Scale Horizontal` and `Move Aim Scale Vertical` default to 100%.
`lightgun_aim_scale.h` applies centre-relative scaling to the normalized mouse
coordinates consistently for pose and image-state conversion. Off-screen edge
coordinates remain available. Scale controls are exposed in the profile UI.

## Tests and practical limits

The repository `.ci` folder contains standalone C++ checks for the Showtime
trajectory, gesture selection, up/down pose, profile device filtering and aim
scale. Compile them with the source include paths; the device-profile test also
requires yaml-cpp. Development harnesses additionally checked angular velocity,
stationary input, quaternion sign changes, time gaps/rewinds, and mixed bindings.
The full Windows build was successful using the workspace setup.

Aiming and The Shoot gestures have user gameplay confirmation; Overkill scale
values still need fine tuning. Two-controller behavior requires hardware testing
on the recipient's guns. Keyboard raw-device isolation is not implemented.
Temporary one-per-second pose logs remain for the two retail title IDs.
Save-state input restoration remains deferred and must not be described as fixed.


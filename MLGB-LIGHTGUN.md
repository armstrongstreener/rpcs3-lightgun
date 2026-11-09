# MLGB RPCS3 Lightgun development build

Based on RPCS3 0.0.43-20573, upstream commit
`47599599a4cea37f350eb0936d7c5e4de6f7bf57`.

This experimental fork adds two alternative physical inputs per virtual mouse
button, plus title-specific raw input profiles. Either input can activate the
button; both do not need to be pressed together. Releasing one keeps the action
held while the other is still pressed.

## Configuration

1. Extract a test build into its own folder. Keep your working emulator and saves.
2. Configure the game's Move handler for Raw Mouse using the existing RPCS3
   settings. The new build does not turn this on automatically.
3. Open **Configuration > Mice > Raw Mouse**. Under Global defaults, select the
   appropriate gun for each player. The two controls in each button row are the
   primary and alternative physical inputs. Each accepts a mouse button or key.
   Right-click a control to clear that binding.
4. Use **Configuration > PS Move > Mouse** to map the virtual mouse buttons to
   Move actions. This separate action mapping remains global.
5. To change raw input bindings for one game, click **Profile: Global defaults**
   and enter its nine-character title ID, such as `BCUS98222` for The Shoot USA.
   Save the profile. It will load automatically when that title starts.
   Leave the title ID blank to edit global defaults again.

Game files live in `config/input_configs/lightgun/TITLE/raw_mouse.yml`.
Missing game profiles inherit `config/raw_mouse.yml`. Existing primary mappings
remain supported. Profiles currently include physical gun assignments, so select
the new devices again when changing guns in an existing profile.

Keyboard input retains upstream's shared keyboard handling: it is not isolated
by physical keyboard device. Assign distinct key codes to different players to
avoid a shared key triggering both players.

## Validation and remaining work

Focused MSVC tests of the edited production event loops passed overlapping
mouse/key inputs, both release orders, key auto-repeat, two-key and two-mouse
alternatives, empty bindings and separate gun instances. This does not establish
GUI correctness or working gameplay. Test both XGUNNERs independently in game.

The Windows workflow builds and packages on GitHub-hosted machines. Its SDK
versions and preparation scripts follow this upstream revision's Windows CI.
It does not install development tools on the user's PC or publish a release.

Save-state changes, keyboard-device isolation, automatic gun assignment and
additional emulator branding are not implemented in this checkpoint.

The changes were prepared with AI assistance and require continued build and
gameplay validation. RPCS3's existing license files and source history apply.

## Game profile screen

On Windows, right-click a stopped game and choose **Configure Lightgun Profile**.
The profile opens for that game's title ID. Rows show the actual PS Move actions
currently routed to each virtual mouse button, alongside Primary and Alternative
bindings. Optional game action labels are editable notes and are saved per profile;
they do not change the routed PS Move button or establish a verified game control.

The Shoot USA (BCUS98222) is the first acceptance target. Verify camera detection,
P1 calibration, menu control and gameplay before testing P2 and any game-specific
motion requirements. No calibration bypass or game-specific motion fix is included.


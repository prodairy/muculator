# Muculator

Pass-through VST3/AU/Standalone plugin (JUCE 9, C++20) with tempo-synced music-production calculators.
Public, AGPLv3, repo `prodairy/muculator`. The owner is new to programming and git: explain steps
in plain Turkish, keep the UI text in English.

## Working with the owner

- **Planned work lives in `ROADMAP.md` (local only, git-ignored; never commit it).** Read it when the owner names a feature or bug ("let's do the host sync item").
- **Do not start coding unless the owner asks for it explicitly.** When they share ideas, bug reports or wishes, write them into `ROADMAP.md` (marked 🧑 for the owner, 🤖 for Claude) and stop. Implement only after an explicit "yapalım" / "do it" for that item.
- Before risky or public actions (force-push, release tags, publishing), explain what will happen and wait for a yes.

## Build & test

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release   # JUCE from $HOME/Downloads/JUCE, else fetched (tag 9.0.2)
cmake --build build
./build/CalcTests                                          # must stay green; add a check for every formula change
./build/MuculatorSnapshot_artefacts/Release/MuculatorSnapshot shots   # renders every screen to PNG
auval -v aufx Mclc Mucu                                   # AU validation (macOS)
```

- On macOS the build copies the plugins into `~/Library/Audio/Plug-Ins/{VST3,Components}` (`MUCULATOR_INSTALL_PLUGIN`).
- On Windows, AU is skipped and the copy is off (the VST3 folder needs admin rights).
- Universal binary (arm64 + x86_64): `CMAKE_OSX_ARCHITECTURES` must stay **before** `project()`.
- After any UI change, render snapshots and look at them; do not trust a successful compile alone.

## Layout

| Path | Role |
|---|---|
| `Source/Calc.{h,cpp}` | All maths. Pure C++, **no JUCE**, unit tested by `Tests/CalcTests.cpp`. |
| `Source/Modules/ModuleBase.*` | `ModuleContext` (BPM, chain, sample rate, per-module ValueTree) and the `Validator` helper. |
| `Source/Modules/*Module.cpp` | One module each: Reverb, Delay, Compressor, Pitch (4 tabs), Converter, Loop. All declared in `Modules.h`. |
| `Source/UI/Skin.*` | `Skin` (every colour, button style, corner, texture, font), `.muskin` JSON parser, `SkinManager` (built-in + user skins, saved choice). `ui::skin()` is the current skin. |
| `Source/UI/ConsoleLookAndFeel.*` | Draws everything from `ui::skin()`: fonts, `drawSection`, `drawScreen`, `drawLed`, the four button styles. |
| `Skins/*.muskin` | Built-in skins, embedded with `juce_add_binary_data`: 5 studio skins and 5 colour skins from Sanzo Wada's *A Dictionary of Color Combinations* (`SkinManager::colourSkinIds`). Format and the Claude prompt: `docs/SKINS.md`. |
| `Source/UI/Widgets.*` | `IllumButton`, `ChainButton`, `LedField` (drag / wheel via `setNudger`), `NotePicker`, `LedReadout`, `LedTable` (scrolls; click copies a value), `WarningPanel`, `HelpPanel`, `ButtonGroup`. |
| `Source/PluginEditor.*` | Top bar (SKIN menu, BPM, TAP, CHAIN, HOST SYNC), module switcher, implements `ModuleContext`. |
| `Source/PluginProcessor.*` | Pass-through audio; reads host BPM and sample rate into atomics; stores state as a ValueTree. |
| `Tests/Snapshot.cpp` | Headless renderer + invalid-input smoke test. |
| `.github/workflows/build.yml` | Windows + macOS CI: unit tests, snapshot smoke test (PNGs uploaded as `Snapshots-*` artifacts), pluginval (VST3, strictness 10), auval (AU). A `v*` tag publishes a GitHub Release with zips. |

## Decisions (do not change without asking)

- 12-TET, C4 = MIDI 60. **A4 = 440 Hz by default**; the Pitch module's `a4` property (400 to 480 Hz) changes it, and every pitch formula in `Calc` takes it as a parameter. Sharps by default; Pitch `flats` switches to Db, Eb...
- Harmonic count = rows listed after the odd / even filter (16 odd = harmonics 1 to 31).
- Stretch is **varispeed**: pitch ratio = new BPM / old BPM. Microtone warning when the **displayed** deviation (rounded to 0.1 ct) is ≥ `kMicrotoneThresholdCents` (10). The warning names both neighbour notes, the cents off each, and the BPM needed to land on each.
- BPM is valid from 1 to 999. Number parsing accepts a comma as the decimal separator. Invalid input must never crash: mark the field red (`LedField::setInvalid`), show the error through `showErrors`, and display `--`.
- CHAIN on: one shared `globalBpm`. CHAIN off: each module keeps its own `bpm` child property, and the top-bar BPM edits the visible module. HOST SYNC locks the BPM field and is applied again when the visible module changes; TAP turns sync off.
- The host may stop calling `processBlock` (e.g. silent tracks). After 1 s without a block the HOST TEMPO readout shows IDLE and, with HOST SYNC on, the footer shows a hint. Logic (detected with `juce::PluginHostType().isLogic()`) pauses every track that is not playing, so its hint says to put Muculator on the Stereo Out; naming Logic in that hint was approved by the owner. Reading the tempo outside `processBlock` is not supported, so this cannot be fixed in code.
- Sample-rate fields (Converter, Loop) follow the host while their HOST SR button is lit; typing a rate turns it off.
- Only one module is visible at a time. Fixed window size of 1000×680.
- Reverb pre-delay is snapped to a 1/64 to 1/512 note and is at most 25 % of the total. Room size uses Sabine with 1.59 : 1.26 : 1 proportions.
- State: modules read and write their own ValueTree child. `setStateInformation` merges into the existing tree and bumps `loadCount` so an open editor rebuilds.
- Skins: never hard-code a colour; use `ui::skin()`. Buttons take a `ui::Cap` role, not a colour. Text on panels uses `text` / `textDim`, text on screens `screenText` / `led*`; LED colours only ever sit on `screen` (warning panels fill with it too). New colour skins take their palette from a numbered Wada combination and name it in the description. The skin is a per-computer setting (`Muculator.settings` next to the `Skins` folder), not part of the DAW project. `Skins/console.muskin` must match `Skin::console()`; the snapshot tool checks it and renders every built-in skin (`skin_<id>_*.png`).
- Avoid trademarks in code and docs (no "SSL"; say "console").

## Conventions

- Shared UI code (`Source/UI/Skin.*`, `ConsoleLookAndFeel.*`, `Widgets.*`) is meant to move to a common base for other plugins later (ROADMAP "Ortak temel"), so do not add Muculator-specific logic to it; BPM, notes and module behaviour belong in `Source/Modules/`, `PluginEditor` or `Calc`. Known exceptions to untangle at that move: `NotePicker` and `copyableValue` use `Calc`.
- JUCE code style: 4 spaces, a space before `(`, `juce::` prefixes, no exceptions for control flow.
- New formula → `Calc.cpp` + a test. New module → subclass `ModuleBase` (including `helpText()` for the **?** panel), add it in `MuculatorEditor::buildModules`, and update `moduleIds` / `moduleNames` and the snapshot tool.
- Commits from this machine: name `prodairy`, email `344521037+prodairy@users.noreply.github.com`. The owner's other public identity (`airy` / `airysocial@pm.me`, used by GitHub web merges) is intentional and fine to leave visible; do not rewrite history to remove it.
- Work on a branch, open a PR, and let CI pass. Release by tagging `vX.Y.Z` on `main`.

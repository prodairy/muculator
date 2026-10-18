# Contributing to Muculator

Thanks for helping! Türkçe ya da İngilizce yazabilirsin.

## Reporting a bug

Open an issue and include:
- your OS and DAW (with versions),
- the module and the values you typed,
- what you expected and what you got (a screenshot helps).

## Changing the maths

All formulas live in `Source/Calc.cpp` (plain C++, no JUCE).
If you change or add one, add a check to `Tests/CalcTests.cpp` and make sure it passes:

```bash
cmake --build build --target CalcTests && ./build/CalcTests
```

## Changing the UI

Modules are in `Source/Modules/`, shared widgets in `Source/UI/`.
Run `MuculatorSnapshot <folder>` to render every screen to PNG and check your change.
Please keep the console look: dark panel, LED readouts, lit buttons.

## Pull requests

- One topic per PR, with a short description of what changed and why.
- Follow the existing code style (JUCE style: 4 spaces, a space before `(`).
- By contributing you agree your code is released under the project's AGPLv3 license.

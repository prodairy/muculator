# Muculator skins

A skin changes how Muculator looks: colours, button style, corner shapes, panel texture and fonts.
It never changes what Muculator calculates.

A skin is one small text file with the **`.muskin`** extension. Inside is JSON, so you can open and
edit it with any text editor. Muculator comes with five built-in skins, and you can add as many of
your own as you like.

| Built-in skin | Look |
|---|---|
| **Console** (default) | Analog mixing console: grey metal, LED screens, illuminated buttons |
| **Pattern** | Step-sequencer studio: blue-grey body, glossy buttons, orange and lime accents |
| **Session** | Minimal clip-based studio: flat light grey, thin lines, light screens |
| **Modular** | Modern modular studio: anthracite, one orange accent, rounded corners |
| **Studio Pro** | Classic desktop studio: graphite, blue accent, soft gradients, glass buttons |

The built-in files are in the [`Skins/`](../Skins) folder of this repository. They are good
starting points to copy.

---

## Choosing a skin

Click **SKIN** in the top bar, next to the Muculator logo. The menu lists:

- **BUILT-IN SKINS**: the five skins above.
- **MY SKINS**: every `.muskin` file in your skins folder.
- **Open skins folder**: opens the folder where your own skins go (it is created if needed).
- **Reload skins**: reads the folder again. Use it after adding or editing a file; Muculator stays open.
- **Save this skin as a new file to edit**: writes the skin you see now, with every value filled in,
  into your skins folder. This is the easiest way to start your own.
- **How to make a skin**: opens this page.

The skin is a personal setting stored on your computer, not in the DAW project. Every Muculator
window uses the same skin, and a project you send to someone opens in their own skin.

### Where the skins folder is

| System | Folder |
|---|---|
| macOS | `~/Library/Application Support/Muculator/Skins` |
| Windows | `%APPDATA%\Muculator\Skins` |
| Linux | `~/.config/Muculator/Skins` |

---

## Make a skin with Claude (or another AI assistant)

1. Copy this whole page (or send the link if the assistant can read web pages).
2. Add the prompt below and describe the look you want.
3. Save the answer as a file ending in `.muskin`, for example `Synthwave.muskin`, in your skins folder.
4. In Muculator: **SKIN → Reload skins**, then pick it from **MY SKINS**.

```text
Make a skin for the Muculator audio plugin, following the "File format" section of the guide above.
The look I want: <describe it, e.g. "dark purple synthwave with pink and cyan neon lights">.

Rules:
- Answer with a single JSON code block: the complete .muskin file, every key filled in.
- Keep text readable: "text" and "textDim" on "panel" and "recess", "screenText" and the four
  "led" colours on "screen", "buttonText" on "buttonFace", "litText" on every "cap" colour.
- Keep the meaning of the LED colours: ledRed = errors, ledAmber = values, ledGreen = results,
  ledBlue = information.
- Leave "fonts" empty unless I name a font.
```

If something in the file is wrong, Muculator still loads it: the menu shows the skin with
"(2 problems)" next to its name, and choosing it lists what could not be used. Paste that list back
to the assistant and ask it to fix the file.

---

## File format

A `.muskin` file is a JSON object. Every key is optional: anything left out keeps the value of the
**base** skin, which is Console unless the file says otherwise. Here is the complete Console skin:

```json
{
  "format": 1,
  "name": "Console",
  "author": "Muculator",
  "description": "Analog mixing console: grey metal, LED screens, illuminated buttons.",
  "colors": {
    "panel": "#3a3d42",
    "panelDark": "#2a2c30",
    "panelEdge": "#1a1b1e",
    "recess": "#202225",
    "accent": "#e0342a",
    "text": "#e6e4dc",
    "textDim": "#9a9890",
    "highlight": "#f2c200",
    "screen": "#0a0b0c",
    "screenEdge": "#000000",
    "screenText": "#9a9890",
    "ledRed": "#ff3b2e",
    "ledAmber": "#ffb020",
    "ledGreen": "#3dff86",
    "ledBlue": "#48a8ff",
    "capRed": "#e0342a",
    "capYellow": "#f2c200",
    "capGreen": "#27c24c",
    "capBlue": "#2f86ff",
    "capWhite": "#eeeeea",
    "capOrange": "#ff7a1a",
    "buttonFace": "#2c2e32",
    "buttonText": "#e6e4dc",
    "litText": "#141414"
  },
  "style": {
    "buttons": "glow",
    "texture": "brushed",
    "buttonCorner": 3.5,
    "sectionCorner": 5,
    "screenCorner": 3,
    "accentStripe": 3,
    "screws": true,
    "screenGlare": true
  },
  "fonts": {
    "label": "",
    "led": ""
  }
}
```

### Top level

| Key | Meaning |
|---|---|
| `format` | Always `1` for now. |
| `name` | Name shown in the SKIN menu. If missing, the file name is used. |
| `author`, `description` | Free text, for people who share skins. |
| `base` | Optional: a built-in skin to start from instead of Console: `console`, `pattern`, `session`, `modular` or `studio-pro`. Useful for small variations, e.g. `{ "base": "modular", "colors": { "accent": "#00c2ff" } }`. |

### `colors`

Colours are written `#RRGGBB`, or `#AARRGGBB` with transparency first.

| Key | Where it is used |
|---|---|
| `panel` | Window background. |
| `panelDark` | Top bar, help panel, menus and tooltips. |
| `panelEdge` | Outlines, frames around sections, button bezels. |
| `recess` | Inside of the framed sections (STYLE, RESULT, ...). |
| `accent` | Stripe under the top bar. |
| `text` | Labels on panels: logo, section titles, field captions. |
| `textDim` | Secondary labels on panels: footer, units, hints. |
| `highlight` | Headings in the help panel, the HOST badge on the BPM field. |
| `screen` | Background of the LED screens, tables and input fields. |
| `screenEdge` | Frame around every screen. |
| `screenText` | Labels and column headers on screens. |
| `ledRed` | Errors, invalid input, notes out of tune. |
| `ledAmber` | Values, typed numbers, the recommended row. |
| `ledGreen` | Results, selected rows, the HOST TEMPO readout. |
| `ledBlue` | Information panels and the help panel frame. |
| `capRed` | Lit TAP button. |
| `capYellow` | Lit CHAIN and waveform buttons; menu highlight. |
| `capGreen` | Lit HOST SYNC, HOST SR, NOTE / FREQ buttons. |
| `capBlue` | Lit reverb style buttons and **?** buttons. |
| `capWhite` | Lit module, tab and most other buttons. |
| `capOrange` | Lit compressor source buttons. |
| `buttonFace` | Face of an unlit button (styles `flat`, `gloss`, `glass`; `glow` uses a dark tint of the cap colour instead). |
| `buttonText` | Text on an unlit button. |
| `litText` | Text on a lit button; must be readable on every cap colour. |

### `style`

| Key | Values | Meaning |
|---|---|---|
| `buttons` | `glow`, `flat`, `gloss`, `glass` | `glow`: console buttons that light up from inside. `flat`: solid colour with a thin outline. `gloss`: shiny plastic with a bright upper half. `glass`: soft gradient with a glassy sheen. |
| `texture` | `brushed`, `gradient`, `flat` | Window background: brushed metal lines, a smooth vertical gradient, or one plain colour. |
| `buttonCorner` | 0 to 12 | Corner radius of buttons, in pixels. |
| `sectionCorner` | 0 to 16 | Corner radius of the framed sections. |
| `screenCorner` | 0 to 10 | Corner radius of screens and input fields. |
| `accentStripe` | 0 to 8 | Height of the accent stripe under the top bar; `0` hides it. |
| `screws` | `true` / `false` | Screws in the four window corners. |
| `screenGlare` | `true` / `false` | Faint reflection on the top half of the screens. |

### `fonts`

| Key | Meaning |
|---|---|
| `label` | Font for labels and buttons. |
| `led` | Font for the LED screens; a monospaced font keeps the numbers aligned. |

Leave a font empty (`""`) to use the system default. A font must be installed on the computer;
if it is not, the system picks a replacement. The window size is fixed, so very wide fonts may cut
off long labels.

---

## Tips

- Start from the built-in skin closest to what you want, then change a few colours at a time.
- Light skins work (see Session): use a light `screen` and darker LED colours.
- Test every screen: the Pitch module's HARMONICS tab, a help panel (**?**) and an invalid BPM
  (type `0`) show most colours at once.
- Muculator never crashes on a bad file. An unreadable file shows as "(unreadable file)" in the
  menu; a wrong value keeps the base skin's value and is listed when you choose the skin.

## Adding a built-in skin to Muculator

Pull requests with new skins are welcome. Add the file to `Skins/`, list it in
`juce_add_binary_data` in `CMakeLists.txt` and in `SkinManager::builtInIds()` in
`Source/UI/Skin.cpp`. CI checks that every built-in skin loads without problems and uploads
screenshots of it (`skin_<name>_*.png`). Use names that describe a look, not a product or brand.

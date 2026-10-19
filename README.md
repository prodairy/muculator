<div align="center">

# MUCULATOR

**The calculator rack for music producers.**
Free, open-source VST3 / AU / CLAP plugin (and standalone app) that does the studio maths for you: reverb and delay times, compressor timing, pitch after time-stretch, unit conversions and loop lengths, all locked to your tempo.

![Stretch module showing a microtone warning](docs/screenshots/stretch-microtone.png)

[**Download**](../../releases/latest) · [Features](#features) · [Build from source](#build-from-source) · [Türkçe](#türkçe)

</div>

---

## Why?

Every producer keeps a browser tab open for "BPM to ms", "what note is 462 Hz" or "what pre-delay for a hall at 128 BPM".
Muculator puts all of that inside your DAW, styled like a console channel strip: dark panel, LED displays, lit buttons.

- **Audio passes through untouched.** Drop it on any track; it costs no DSP.
- **Follows your DAW tempo** (Host Sync), or tap it in (button or the `T` key). Drag or scroll the BPM display to change it; hold Shift for 0.1 steps.
- **Chain mode:** one BPM drives every module, or unlink and give each module its own.
- **Never crashes on bad input.** Type `0`, `abc` or leave a field empty and you get a clear warning instead.
- **Click any value to copy it**, then paste it into your EQ, delay or synth.
- **Built-in help:** the **?** button on every module explains what it does, how to use it and the terms it uses.

## Features

| Module | What it tells you |
|---|---|
| **Reverb** | Pre-delay, decay and total time for Small Room, Room, Large Room, Plate and Hall, synced to BPM. The recommended row is highlighted, you can click any row, and it shows an estimated room size (Sabine). |
| **Delay** | Every note value from 1/1 to 1/512: normal, dotted and triplet, in **ms and Hz**. Swing timing for 1/8 and 1/16 (50 to 75 %) and tempo-synced Haas delays (10 to 35 ms). |
| **Compressor** | Attack based on the source (kick, snare, bass, vocal, bus…) and release based on the tempo, with warnings when the release gets too short. |
| **Pitch** | **Transpose** (note ± semitones/cents), **Stretch** (varispeed or time-stretch, with fit to scale), **Frequency → Note**, **Harmonic series** (all / odd / even, saw / square / triangle levels, 16 to 64 harmonics), **Frequency map** (where common instruments sit, for EQ). Adjustable A4 reference (440 Hz by default, 432 / 442 / 443 or any value), always on screen. Notes with sharps or flats. |
| **Converters** | dB ↔ linear gain · EQ Q ↔ octaves ↔ Hz · ms ↔ Hz ↔ samples · distance ↔ ms (mic/speaker alignment) · audio file size (duration, sample rate, bit depth, channels → MB). |
| **Loop** | Bars + time signature → ms, seconds and exact sample count. **Length → BPM:** the tempo of a loop from its length (seconds, ms or samples) and bar count. |

### The Stretch module

Changed the tempo of a sample? Enter the **old BPM**, the **old note** (or frequency) and the **new BPM**.
Muculator gives you the new note and frequency. If the result lands **between two notes** (10 cents or more off the nearest note), you get a **MICROTONE WARNING** telling you:

- which two notes it sits between,
- how many cents it is off from each,
- **which BPM to use** so the sample lands exactly on a note.

> Example: an A4 sample at 120 BPM played at 126 BPM → 462 Hz, −15.5 ct from A#4. Use **127.14 BPM** to land on A#4.

**Fit to scale:** pick the key and scale of your song (major or minor) and Muculator lists the nearest tempos that land the sample exactly on a note of that scale.

**Time-stretch mode:** if your DAW keeps the pitch (Warp, Flex Time), switch to **TIME-STRETCH**. You get the new length, the tempo change and the stretch ratio, with a warning when the stretch is large enough to be heard (above 10 %, and a quality warning above 25 %).

### Skins

Click **SKIN** in the top bar to change the look. Fifteen skins are built in: five studio skins
(**Console**, the default, **Pattern**, **Session**, **Modular**, **Studio Pro**) and five minimal pastel
skins built from Sanzo Wada's *A Dictionary of Color Combinations* (**Seashell**, **Glaucous**,
**Lavender**, **Lemon**, **Turquoise**), each also in a dark version. A skin is a small `.muskin` text file, so
you can make your own, or ask Claude (or another AI assistant) to make one from a description.
See the [skin guide](docs/SKINS.md).

<details>
<summary><b>More screenshots</b></summary>

| | |
|---|---|
| ![Reverb](docs/screenshots/reverb.png) | ![Delay](docs/screenshots/delay.png) |
| ![Compressor](docs/screenshots/compressor.png) | ![Harmonics](docs/screenshots/harmonics.png) |
| ![Converters](docs/screenshots/converters.png) | ![Loop](docs/screenshots/loop.png) |

</details>

## Install

1. Download the zip for your system from [**Releases**](../../releases/latest).
2. Copy the plugin to:
   - **Windows (VST3):** `C:\Program Files\Common Files\VST3\`
   - **macOS (VST3):** `~/Library/Audio/Plug-Ins/VST3/`
   - **macOS (AU):** `~/Library/Audio/Plug-Ins/Components/`
   - **Windows (CLAP):** `C:\Program Files\Common Files\CLAP\`
   - **macOS (CLAP):** `~/Library/Audio/Plug-Ins/CLAP/`
3. Rescan plugins in your DAW. Muculator shows up as an effect.

**Standalone app (macOS):** `Muculator.app` opens without a DAW. There is no host there, so HOST SYNC and HOST SR are hidden: set the tempo by typing, dragging or TAP, and type the sample rate.

> **macOS:** the builds are not notarized yet. If macOS blocks the plugin, run
> `xattr -cr ~/Library/Audio/Plug-Ins/VST3/Muculator.vst3` (and the same for `Muculator.component`, `Muculator.clap` and `Muculator.app`).
>
> **Windows:** SmartScreen may warn about an unknown publisher; the plugin is unsigned.

> **Host Sync in Logic:** Logic pauses plugins on tracks that are not playing, and Muculator can only read the tempo while it runs.
> Put Muculator on the **Stereo Out** so Host Sync follows the tempo at all times, even when stopped. It does not touch the audio and uses no DSP, so this costs nothing.
> Other hosts that pause silent tracks work the same way on the master channel.

## Build from source

You need CMake ≥ 3.22 and a C++20 compiler (Xcode / Command Line Tools on macOS, Visual Studio 2022 or newer on Windows).
JUCE is downloaded automatically if it is not found locally.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Artefacts end up in `build/Muculator_artefacts/Release/` (`VST3/`, `AU/`, `CLAP/`, `Standalone/`). The CLAP target comes from [clap-juce-extensions](https://github.com/free-audio/clap-juce-extensions) (MIT), downloaded at configure time; turn it off with `-DMUCULATOR_CLAP=OFF`.

| Target | Purpose |
|---|---|
| `CalcTests` | Unit tests for every formula (`Source/Calc.cpp`, no JUCE dependency) |
| `MuculatorSnapshot` | Renders every screen to PNG, used to make the screenshots above |

Project layout:

```
Source/Calc.*            all maths: pure C++, unit tested
Source/Modules/          one file per module (Reverb, Delay, …)
Source/UI/               console look & feel, LED fields, lit buttons, tables
Source/Plugin*.cpp       pass-through processor + editor shell
Tests/                   unit tests + snapshot tool
```

## Assumptions & formulas

- 12-TET, A4 = 440 Hz by default (adjustable from 400 to 480 Hz), C4 = MIDI 60, notes written with sharps unless you pick flats.
- Ideal waveform harmonics: saw 1/n (all), square 1/n (odd), triangle 1/n² (odd).
- Stretch = varispeed (tape-style): pitch ratio = new BPM / old BPM. Time-stretch keeps the pitch: new length = old length × old BPM / new BPM; up to 10 % tempo change is rated clean, above 25 % gets a quality warning.
- Loop tempo = bars × beats per bar × (4 / beat unit) × 60 / length in seconds.
- Swing: the first note of each pair takes the swing % of the pair (50 % = straight, 66.7 % = triplet shuffle).
- File size = duration × sample rate × bit depth / 8 × channels (uncompressed PCM, header not counted).
- The frequency map shows typical ranges from common EQ charts; real recordings vary.
- Microtone warning when the shown deviation is ≥ 10 cents.
- Reverb pre-delay is snapped to the 1/64 to 1/512 note closest to each style's typical value, and is never more than 25 % of the tail. Room size comes from Sabine's formula with 1.59 : 1.26 : 1 proportions.
- Compressor suggestions are common starting points, not rules.
- Speed of sound = 331.3·√(1 + T/273.15) m/s.

## Contributing

Bug reports, ideas and pull requests are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

Copyright © 2026 prodairy. Muculator is free software under the **GNU AGPLv3** ([LICENSE](LICENSE)).
It is built with [JUCE](https://juce.com) (AGPLv3), the Steinberg VST3 SDK (MIT) and [clap-juce-extensions](https://github.com/free-audio/clap-juce-extensions) (MIT).
VST is a registered trademark of Steinberg Media Technologies GmbH.

---

## Türkçe

**Muculator**, müzik prodüktörleri için ücretsiz ve açık kaynak bir VST3 / AU / CLAP eklentisidir (tek başına açılan bir uygulaması da var). Sesi değiştirmez; stüdyodaki hesapları DAW'ın içinde, tempona kilitli olarak yapar.

- **Reverb:** Tarza ve BPM'e göre pre-delay, decay ve toplam süre; önerilen satır işaretli; tahmini oda ölçüleri.
- **Delay:** 1/1'den 1/512'ye kadar normal, noktalı ve triplet süreler (ms ve Hz); 1/8 ve 1/16 için swing süreleri (%50-75) ve tempoya uyan Haas gecikmeleri (10-35 ms).
- **Compressor:** Kaynağa göre attack, BPM'e göre release.
- **Pitch:** Transpose, Stretch (varispeed ya da time-stretch, gama oturtma ile), frekanstan nota bulma, enstrüman frekans haritası (EQ rehberi), harmonik serisi (tümü / tek / çift, saw / square / triangle seviyeleri, 16-64 harmonik). A4 referansı ayarlanabilir (varsayılan 440 Hz) ve hep ekranda; notalar diyez ya da bemol yazılabilir.
- **Kolaylıklar:** Her modülde **?** yardım paneli; bir değere tıklayınca panoya kopyalanır; BPM ekranı sürükleyerek ya da tekerlekle değişir (Shift ile 0.1 adım); `T` tuşu TAP yapar.
- **Converters:** dB ↔ gain, Q ↔ oktav ↔ Hz, ms ↔ Hz ↔ sample, mesafe ↔ ms, ses dosyası boyutu (süre, sample rate, bit, kanal → MB).
- **Loop:** Bar sayısından ms, saniye ve sample sayısı. Ters yönde de çalışır: loop'un uzunluğunu ve bar sayısını gir, BPM'ini bul.
- **Skin'ler:** Üst bardaki **SKIN** düğmesiyle görünüm değişir: 5 stüdyo skin'i (Console, Pattern, Session, Modular, Studio Pro) ve Sanzo Wada'nın *A Dictionary of Color Combinations* kitabındaki renklerle yapılmış 5 sade, pastel skin (Seashell, Glaucous, Lavender, Lemon, Turquoise); her birinin bir de koyu (Dark) versiyonu var. Skin, `.muskin` uzantılı küçük bir metin dosyasıdır; kendin yapabilir ya da [skin rehberini](docs/SKINS.md) Claude'a verip istediğin görünümü anlatarak yaptırabilirsin.

**Stretch:** Eski BPM'i, eski notayı ve yeni BPM'i girersin; yeni notayı ve frekansı verir. Sonuç iki notanın arasında kalırsa **MİKROTON UYARISI** gösterir ve notaya tam oturması için hangi BPM'i kullanman gerektiğini yazar. **Gama oturt:** şarkının tonunu ve gamını seç; sample'ı o gamdaki bir notaya tam oturtan en yakın BPM'leri listeler. Perde sabit kalsın istiyorsan **TIME-STRETCH** moduna geç: yeni uzunluğu, tempo değişimini ve oran çok büyükse kalite uyarısını gösterir.

**Kurulum:** [Releases](../../releases/latest) sayfasından kendi sistemine uygun zip dosyasını indir, eklentiyi yukarıdaki klasöre kopyala ve DAW'da eklentileri yeniden tarat.

**Standalone uygulama (macOS):** `Muculator.app` DAW olmadan açılır. Orada DAW olmadığı için HOST SYNC ve HOST SR düğmeleri gizlidir: tempoyu yazarak, sürükleyerek ya da TAP ile gir, sample rate'i elle yaz.

**Logic'te Host Sync:** Logic, çalmayan kanallardaki eklentileri durdurur; Muculator tempoyu yalnızca çalışırken okuyabilir. Host Sync'in her zaman, durdurulmuşken bile çalışması için Muculator'ı **Stereo Out** kanalına koy. Sese dokunmadığı ve işlemci harcamadığı için bunun bir zararı yok.

# UIX Lite XOnline Dashboard patch

This directory contains the separate, hash-locked patcher for the retail
`xonlinedash.xbe`. No patched Microsoft XBE is distributed.

## Patching

You need Python 3 and the supported retail XOnline Dashboard XBE (SHA-256
`FFE013D6067025281BC3C9BC9F9B62C99107BAAFDFB1005C2E98AE6DBC74714D`).

```text
python patch.py xonlinedash.xbe xonline_uix.xbe
```

The input is never modified. All features are enabled by default. Select
features with `--only` or remove features with `--skip`; names are
comma-separated.

| Feature | What it does |
|---|---|
| `colors` | Loads matching `Color`, `ColorA`, and `ColorB` material values from the current UIX skin |
| `textures` | Replaces matching XOnline XBX textures with files from the current UIX skin |
| `video720` | Enables matching 1280x720 device and UI dimensions when HDTV, widescreen, 720p, and NTSC are enabled |
| `redirect` | Returns to `Y:\xb0xdash.xbe` when it exists; otherwise preserves the retail `xboxdash.xbe` return |
| `fontpath` | Changes the font path from `\Device\Harddisk0\partition2\xodash\Xbox.xtf` to `\Device\Harddisk0\partition2\Xbox.xtf` |

Examples:

```text
python patch.py --skip video720 xonlinedash.xbe xonline_uix.xbe
python patch.py --only colors,textures xonlinedash.xbe xonline_skin.xbe
```

Keep generated filenames below the FATX 42-character limit.

## Skin behavior

The patch reads `[Dashboard Settings] Current Skin` from
`T:\UIXLite\config.ini` when XOnline Dashboard starts. `T:` is the dashboard
title-data mapping for `\Device\Harddisk0\Partition1\TDATA\fffe0000\`. The selected
The `T:\UIXLite` and `T:\UIXLite\Skins` directories are created automatically
when needed. The selected skin is loaded from `T:\UIXLite\Skins\<Name>\`.

The color hooks apply `Color`, `ColorA`, and `ColorB` values to matching
XOnline material names. The texture hooks replace files normally loaded from
`B:\Media\Xbx` when a same-name XBX exists in the selected skin folder.
Missing color and texture overrides retain their retail values. XBG models are
not redirected. Skin changes are applied on the next XOnline Dashboard entry;
live switching inside XOnline Dashboard is intentionally unsupported.

The redirect feature changes only the filename passed to the original
lower-level return launcher. All other retail launch arguments are retained.
If `Y:\xb0xdash.xbe` is absent, XOnline returns to `xboxdash.xbe`.

## Building the payload

Prebuilt payload files are included in `bin/`, so ordinary patching requires
only Python. Rebuild after changing `src/xonline_skin.c` with LLVM
(`clang`, `ld.lld`, `llvm-objcopy`, and `llvm-nm`):

Windows:

```text
build.bat
```

macOS or Linux:

```sh
./build.sh
```

The payload is linked at `0x00158D00` and placed into a verified empty cave in
the supported retail XBE. The patcher checks every original hook byte before
writing the output.


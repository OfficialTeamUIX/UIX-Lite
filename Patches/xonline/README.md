# UIX Lite XOnline Dashboard patch

This directory contains the source, hash-locked command-line patcher, and
VisualXIP package for applying the UIX Lite changes to a supported retail
`xonlinedash.xbe`.

## Patching

You need the supported retail XOnline Dashboard `xonlinedash.xbe` (SHA-256
`FFE013D6067025281BC3C9BC9F9B62C99107BAAFDFB1005C2E98AE6DBC74714D`) and one
of these patchers:

- Python 3 for the command-line `patch.py` patcher; or
- the latest VisualXIP for the graphical `.vxp` patch package.

### Python command line

```text
python patch.py xonlinedash.xbe xonline_uix.xbe
```

### VisualXIP

Open `bin/xonlinedash-uixlite.vxp` in the latest VisualXIP, select the supported
retail `xonlinedash.xbe` as the input, choose the features you want, and save
the patched XBE to a new file. The `.vxp` contains the payload, symbols,
feature definitions, hooks, and in-place edits; it does not contain a patched
Microsoft XBE.

Both methods verify the retail input and do not modify it. All features are
enabled by default. In VisualXIP, select features in the feature list. On the
command line, use `--only` or `--skip` with comma-separated feature names.

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

The patch reads `[default] CurrentSkin` from
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
writing the output. Both build scripts regenerate `bin/xonline_skin.bin`,
`bin/symbols.txt`, and `bin/xonlinedash-uixlite.vxp`; commit all three with
source changes.


# UIX Lite XBE patches

Everything UIX Lite changes in `xboxdash.xbe`, as source. `patch.py` applies it
to your own retail 5960 dashboard; no patched XBE is shipped.

## Patching

You need Python 3 and the retail 5960 `xboxdash.xbe` (from `C:\` on the Xbox).

```
python3 patch.py xboxdash.xbe xboxdash_uixlite.xbe
```

The patcher checks the input is the retail 5960 XBE (MD5
`08d3a6f99184679aa13008d6397bacce`) and refuses anything else, and never
touches the input. Copy the output back to `C:\xboxdash.xbe` along with the
UIX Lite XIPs.

Pick features with `--only` or `--skip` (comma-separated):

| Feature | What it does |
|---|---|
| `drives` | Maps HDD0 partitions 6 and 7 as `N:` and `O:`, and a second hard drive's partitions 6, 7 and 1 as `P:`, `Q:` and `R:` |
| `video720` | 1280x720 progressive on an HDTV pack when 720p and widescreen are enabled (NTSC) |
| `xipsig` | Loads modified XIPs by skipping the signature and digest checks. UIX Lite's XIPs need this |
| `dvdregion` | Plays DVDs from any region |
| `skins` | Theseus-style skins from `C:\UIX Configs\Skins\<Name>\`, switchable live from Settings > Skins |
| `discimages` | Title folders with `default.iso` or `default.cci` show in the Launcher and boot (Cerbios) |
| `ftp` | FTP file server on port 21 for pushing files to the console over the LAN |

```
python3 patch.py --skip video720 xboxdash.xbe xboxdash_uixlite.xbe
```

### FTP

Log in as `xbox` / `xbox` by default. Each drive is a folder at the root,
using the same letters the dashboard's file browser shows (`C`, `E`, `F`,
`G`, `D` for the disc, `E2`/`F2`/`G2` for a second HDD), so `E:\` on the box
is `/E`. It comes up on its own once the network is ready and coexists with
Insignia sign-in.

Configure it in `Y:\UIX Configs\config.ini` (all keys optional):

```
[FTP]
Enabled=1      ; 0 turns the server off
User=xbox      ; login name
Pass=xbox      ; login password
Port=21        ; listen port
```

The dashboard's LAN IP is available to skins as `theConfig.GetIPAddress()`
(returns e.g. `192.168.1.50`), the same way `GetXdashVersion()` works, so a
skin can show people where to point their FTP client.

The XBE's signature isn't redone, so like every UIX Lite build it needs a
modded BIOS.

## How it's put together

| Path | What |
|---|---|
| `src/*.c` | The code the patches add, built into one new section (`.uixlite`, at `0x001fe000`) |
| `src/dash5960.h` | The retail addresses the code calls into |
| `bin/uixlite.bin`, `bin/symbols.txt` | That code, prebuilt by `build.sh`, so patching needs only Python |
| `patch.py` | Adds the section, points the hook sites at it, and makes the in-place edits (listed in the file with the instructions they write) |

Hooks are 5-byte `call`/`jmp` redirects at these sites:

| Site | Retail | Goes to |
|---|---|---|
| `0002dca1` | main starts the dashboard | `patch_start` (`drives.c`): mount the drives, start the FTP server, then start it |
| `0002ce6f` | D3D setup creates the device | `patch_create_device` (`video.c`): adjust the present parameters first |
| `0002d89d` | startup builds the material table | `patch_material_init` (`skins.c`): then apply the current skin's colors |
| `0006172f` | texture cache miss looks in the XIPs | `patch_texture_from_xip` (`skin_textures.c`): swap in the skin's texture |
| `0003a490` | theConfig's script function table | `patch_config_getfunctionmap` (`config_functions.c`): adds `ApplySkin`, `LaunchDiscImage`, `DiscImagesSupported`, `GetIPAddress` (`hostinfo.c`) |

The FTP server is `ftp.c`; it runs on the retail network stack through
`net.c` (listen/accept, which the stack has but doesn't export) and the
socket thunks. No new hook site: `patch_start` starts it.

The XIP and DVD region patches are small in-place edits; `patch.py` lists
each one with the retail bytes it expects and the assembly it writes.

## Building the code

Only needed if you change `src/`. Needs LLVM (`clang`, `ld.lld`,
`llvm-objcopy`, `llvm-nm`):

```
./build.sh
```

That rebuilds `bin/`; commit it with the source change.

## Scripts

The XAP side lives in `Xip Source/`: the Skins menu (`default.xip/skins.xap`,
`settings_panel.xip/default3.xap`, the entry in `settings3.xap`) and disc
image support in the Launcher (`FindLaunchFile` and friends in
`default.xip/default.xap`, `harddrive.xap`). Images only appear when
`theConfig.DiscImagesSupported()` finds Cerbios's virtual drive.

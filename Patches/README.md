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
| `drives` | Maps the CD-ROM internally as `B:` (displayed as `D:` only with media present), HDD0 cache partitions 3/4/5 as `S:`/`V:`/`W:` (displayed as `X:`/`Y:`/`Z:`), partitions 6/7 as `N:`/`O:`, and a second hard drive's partitions 6/7/1 as `P:`/`Q:`/`R:` |
| `video720` | 1280x720 progressive on an HDTV pack when 720p and widescreen are enabled (NTSC) |
| `xipsig` | Loads modified XIPs by skipping the signature and digest checks. UIX Lite's XIPs need this |
| `dvdregion` | Plays DVDs from any region |
| `ram128` | Clears the XBE `Limit64Megabytes` initialization flag, allowing a RAM-upgraded Xbox or emulator to expose memory above 64 MB |
| `skins` | Theseus-style skins from `C:\UIX Configs\Skins\<Name>\`, switchable live from Settings > Skins |
| `discimages` | Title folders with `default.iso` or `default.cci` show in the Launcher and boot (Cerbios) |
| `config` | File operations, XBE title information, and LAN-address functions exposed to XAP |
| `ftp` | FTP file server on port 21 for pushing files to the console over the LAN |

Every XBX texture requested by the 5960 dashboard is eligible for a same-name
override in the selected skin folder. The built-in asset groups additionally
provide compatible alias names. Mesh eligibility remains separately limited.

```
python3 patch.py --skip video720 xboxdash.xbe xboxdash_uixlite.xbe
```

### FTP

FTP starts by default with the scene-standard `xbox` / `xbox` credentials. Each drive is a folder at the root,
using the same letters the dashboard's file browser shows (`C`, `D`, `E`, `F`, `G`, `X`, `Y`,
`Z` for the HDD, `E2`/`F2`/`G2` for a second HDD), so `E:\` on the box is `/E`. 
It comes up on its own once the network is ready and coexists with
Insignia sign-in.

Configure or disable it in `C:\UIX Configs\config.ini` (all keys are optional):

```
[FTP]
Enabled=1      ; 0 disables the server
User=xbox      ; login name
Pass=xbox      ; password
Port=21        ; listen port
```

The dashboard's LAN IP is available to skins as `theConfig.GetIPAddress()`
(returns e.g. `192.168.1.50`), the same way `GetXdashVersion()` works, so a
skin can show people where to point their FTP client.

The server supports active and passive transfers, resume, upload/download,
directory creation/removal, deletion, and file or directory rename through
the standard `RNFR`/`RNTO` command pair. Active-mode targets are restricted to
RFC 1918 private networks (`10/8`, `172.16/12`, and `192.168/16`) to prevent
public FTP bounce targets. Both active and passive data sockets use a tested
32 KiB receive buffer.

A normal `STOR` upload is written to a same-directory temporary file and only
replaces the destination after the complete transfer, so a broken connection
does not truncate an existing file. A short disk write, including a full FATX
partition, returns FTP status `452 Insufficient storage space`; other data
connection failures return `426 Transfer failed`. FTP is cleartext and exposes
writable partitions, so use it only on a trusted LAN.

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
| `0005c221` | a dirty Mesh node reloads its URL | `patch_mesh_load` (`skin_meshes.c`): use the selected skin's `.xm` when present |
| `00050e1a` | the render path performs its initial/dirty Mesh load | `patch_mesh_load` (`skin_meshes.c`): track and skin meshes loaded outside the ordinary update pass |
| `0005c297` | Mesh cache lookup receives the resolved virtual path | `patch_mesh_cache_lookup` (`skin_meshes.c`): save the retail XIP path and substitute the skin path |
| `0005c0c4` | a Mesh node is destroyed | `patch_mesh_destroy` (`skin_meshes.c`): remove the node from live-reload tracking |
| `0003a490` | theConfig's script function table | `patch_config_getfunctionmap` (`config_functions.c`): adds the script functions below |

A selected skin with `cellwall.xm` (or its `Inner_cell-FACES.xm` alias) uses
that mesh. If the file is absent, the saved archive-qualified XIP path restores
the retail mesh synchronously when `ApplySkin()` runs, matching texture fallback
behavior even when the Mesh node is not scheduled for another URL update.

### Script functions added to theConfig

Callable from XAP as `theConfig.<name>(...)`:

| Function | Signature | What it does |
|---|---|---|
| `ApplySkin()` | void | Re-apply the current skin's colors/textures (`skins.c`) |
| `LaunchDiscImage(path)` | int | Boot a `.iso`/`.cci` via Cerbios (`disc_image.c`) |
| `GetDiscImageTitleID(imagePath)` | string | Read the 8-hex TitleID from `default.xbe` inside an XISO or LZ4-compressed CCI; returns empty for invalid images |
| `GetDiscImageTitleName(imagePath)` | string | Read the certificate title from `default.xbe` inside an XISO or LZ4-compressed CCI |
| `ExportDiscImageTitleImage(imagePath, imageDestination)` | bool | Export the raw `$$XTIMAGE` section from `default.xbe` inside an XISO or LZ4-compressed CCI |
| `DiscImagesSupported()` | int | 1 if Cerbios's virtual drive is present |
| `GetIPAddress()` | string | The console's LAN IP, e.g. `192.168.1.50` (`hostinfo.c`) |
| `GetXbeTitleID(path)` | string | 8-hex TitleID from a title's `default.xbe`, e.g. `4D530064` (`fileops.c`) |
| `GetXbeTitleName(path)` | string | Title name from a title's `default.xbe` certificate (`fileops.c`) |
| `ExportXbeTitleImage(sourceXbe, imageDestination)` | bool | Export the source XBE's raw `$$XTIMAGE` section; false if absent or on I/O failure |
| `DeleteFile(path)` | int/bool | Delete a file; 1 on success |
| `CreateDir(path)` | int/bool | Create one directory; parent directories must already exist |
| `RemoveDir(path)` | int/bool | Remove an empty directory |
| `MoveFile(src, dst)` | bool | Move/rename, replacing dst |
| `CopyFile(src, dst)` | bool | Copy the file's bytes |
| `SendDiscordRelay(titleID)` | int | Send a title ID to the configured relay and block until complete |
| `SendDiscordRelayAsync(titleID)` | int | Queue TCP/WebSocket on a worker thread; UDP uses the blocking completion-confirmed path |
| `GetDiscordRelayStatus()` | int | Relay state: 0 idle, 1 sending, 2 sent, or a negative Winsock error |

Relay status uses negative Winsock errors for socket failures. Diagnostic
codes `-20001` through `-20004` identify invalid configuration, network not
ready, worker creation failure, or an unsupported relay port; `-20010` and
`-20011` identify socket creation or `SO_INSECURE` failure without a Winsock
error value.
Diagnostic code `-20013` means the UDP request did not complete within ten
seconds. Codes `-20014` and `-20015` mean its completion status or completed
byte count was invalid.
Configure the relay in `C:\UIX Configs\config.ini`:

```ini
[DiscordRelay]
PresenceEnabled=false
IP=192.168.1.100
Port=1102
```

`PresenceEnabled` is a boolean (`true` or `false`) read and written by the XAP;
the dashboard's **Enable Discord Presence** setting controls this value.
Port `1101` uses WebSocket, `1102` uses UDP, and `1103` uses TCP. The address
must be a numeric IPv4 address. `SendDiscordRelayAsync` is non-blocking for TCP
and WebSocket. UDP waits for network readiness and confirmed transmission on the
calling thread because worker-thread UDP requests do not reliably progress on
this network stack.

The FTP server is `ftp.c`; it runs on the retail network stack through
`net.c` (listen/accept, which the stack has but doesn't export) and the
socket thunks. `ftp_tune.c` contains the appended 32 KiB active/passive data
socket tuning and STOR-result helper; keeping it after the established payload
preserves the validated hook addresses. No new retail hook site is needed:
`patch_start` starts the server.

The XIP and DVD region patches are small in-place edits; `patch.py` lists
each one with the retail bytes it expects and the assembly it writes.

## Building the code

Only needed if you change `src/`. Needs LLVM (`clang`, `ld.lld`,
`llvm-objcopy`, `llvm-nm`):

```
./build.sh
```

That rebuilds `bin/`; commit it with the source change.

## XOnline Dashboard skin patch

`xonline/patch.py` is a separate, hash-locked patcher for the included
`xonlinedash.xbe` (`SHA-256 FFE013D6...714D`). It reads the same
`[Dashboard Settings] Current Skin` value each time XOnline Dashboard starts.
It applies `Color`, `ColorA`, and `ColorB` values to matching XOnline material
names. XBX textures found beside the selected skin's XBX file override files
with the same name under `B:\Media\Xbx`; missing replacements fall back to the
retail texture. `.xbg` models are never redirected, and live switching inside
XOnline Dashboard is intentionally not implemented. When XOnline Dashboard
returns to the main dashboard, the patch checks for `Y:\xb0xdash.xbe` and 
changes only the filename passed to the original lower-level launcher. If the
alternate file is absent, the retail `xboxdash.xbe` filename and all original
launch arguments are preserved. The patch changes the retail font path to
`\Device\Harddisk0\partition2\Xbox.xtf` and enables XOnline's matching 720p
device and UI dimensions when HDTV, widescreen, 720p, and NTSC are enabled.

Build and apply it on Windows with:

```text
xonline\build.bat
python xonline\patch.py xonlinedash.xbe xonlinedash_uix.xbe
```

On macOS or Linux, rebuild the payload and apply the patch with:

```sh
./xonline/build.sh
python3 xonline/patch.py xonlinedash.xbe xonlinedash_uix.xbe
```

The generated filename remains below the Xbox filesystem's 42-character
limit. Always keep the original `xonlinedash.xbe` so the patch can be rebuilt
from its verified retail input.

All XOnline features are enabled by default and can be selected with
`--only` or `--skip`: `colors`, `textures`, `video720`, `redirect`,
and `fontpath`. See [xonline/README.md](xonline/README.md) for the complete
feature table, skin paths, examples, and build details.

## Scripts

The XAP side lives in `Xip Source/`: the Skins menu (`default.xip/skins.xap`,
`settings_panel.xip/default3.xap`, the entry in `settings3.xap`) and disc
image support in the Launcher (`FindLaunchFile` and friends in
`default.xip/default.xap`, `harddrive.xap`). Images only appear when
`theConfig.DiscImagesSupported()` finds Cerbios's virtual drive. When the
dashboard starts after a mounted title exits, UIX detaches the stale Cerbios
virtual disc so the dashboard does not relaunch it.

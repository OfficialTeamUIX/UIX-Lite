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
| `skins` | Theseus-style skins from `C:\UIX Configs\Skins\<Name>\`, switchable live from Settings > Skins |
| `discimages` | Title folders with `default.iso` or `default.cci` show in the Launcher and boot (Cerbios) |
| `config` | File operations, XBE title information, and LAN-address functions exposed to XAP |
| `ftp` | FTP file server on port 21 for pushing files to the console over the LAN |

```
python3 patch.py --skip video720 xboxdash.xbe xboxdash_uixlite.xbe
```

### FTP

FTP starts by default with the scene-standard `xbox` / `xbox` credentials. Each drive is a folder at the root,
using the same letters the dashboard's file browser shows (`C`, `D`, `E`, `F`, `G`, `X`, `Y`,
`Z` for the disc, `E2`/`F2`/`G2` for a second HDD), so `E:\` on the box is `/E`. 
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
public FTP bounce targets. FTP is
cleartext and exposes writable partitions, so enable it only on a trusted LAN.

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
| `0003a490` | theConfig's script function table | `patch_config_getfunctionmap` (`config_functions.c`): adds the script functions below |

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

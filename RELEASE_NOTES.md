# UIX Lite v0.6 (testing)

UIX Lite v0.6 extends the retail Microsoft 5960 dashboard while retaining its
Xbox Live/Insignia compatibility. This remains a testing release: back up the
dashboard before installing it and use it only on a modded Xbox whose boot
method does not depend on the Microsoft dashboard files.

The 5960 and XOnline Dashboard patches are maintained as readable source under
[`Patches/`](Patches). Apply them with Python 3 or the included VisualXIP
`.vxp` packages; a pre-patched release is also available for users who do not
need to build the patches themselves.

## Highlights

- A configurable Launcher for applications, dashboards, games, emulators,
  homebrew, and Cerbios disc images.
- Live dashboard skins with color, texture, background, and mesh overrides.
  Skin settings may use `.ini` or legacy `.xbx` files.
- Stock and REZN8-style main menus, configurable menu items, QuickLaunch, title
  caching, recent titles, and an optional screensaver.
- An in-dashboard FTP server with real file attributes and timestamps, active
  and passive transfers, safe uploads, rename, and large-file resume.
- Discord Presence relay support over UDP, TCP, or WebSocket.
- Asynchronous NTP/SNTP clock synchronization.
- Optional 720p and 128 MB RAM patches.
- A selectable XOnline Dashboard patch for matching skin colors and textures,
  720p, font-path correction, and alternate-dashboard return.

## Dashboard patch features

| Feature | What it does |
|---|---|
| `drives` | Maps the CD-ROM, cache partitions, extended partitions, and supported second-HDD partitions, then warms the extended-partition subsystem on a worker thread |
| `video720` | Enables 1280x720 progressive output with an HDTV pack when NTSC, widescreen, and 720p are enabled |
| `xipsig` | Loads modified XIP archives by bypassing their retail signature and digest checks |
| `dvdregion` | Removes the retail DVD region restriction |
| `ram128` | Clears the XBE `Limit64Megabytes` flag so upgraded hardware and emulators can expose memory above 64 MB |
| `skins` | Loads live-switchable colors, textures, backgrounds, and supported `.xm` meshes from `E:\TDATA\fffe0000\UIXLite\Skins\<Name>\` |
| `discimages` | Finds and launches `default.iso` and LZ4-compressed `default.cci` title folders through Cerbios |
| `config` | Adds file, title metadata, cache, network, Discord relay, NTP, and disc-image functions to `theConfig` |
| `ftp` | Runs the built-in FTP server after dashboard drive and network initialization |

All dashboard features are enabled by default and can be selected with
`--only` or `--skip` in `patch.py`, or with the feature list in VisualXIP.

## Skinning and interface

- Settings now live in `E:\TDATA\fffe0000\UIXLite\config.ini`.
- `E:\TDATA\fffe0000\UIXLite` and its `Skins` directory are created during
  dashboard startup.
- A skin can be stored as `<Name>.ini` or `<Name>.xbx`; `.ini` takes precedence
  when both are present.
- Dashboard XBX textures can be replaced by same-name files in the selected
  skin. Tracked mesh nodes can switch to skin `.xm` files and restore their
  archive-qualified retail paths when an override is removed.
- Backgrounds and layer meshes can be changed without rebooting when the XAP
  keeps the relevant nodes active for loading.
- The REZN8 main-menu option includes animated or static orbs, Launcher
  integration, an analog-clock screensaver, and coordinated Info Panel state.
- The stock main menu, tHc orb option, customizable actions, four-button
  QuickLaunch, title icons, title caching, and recent-title launch remain
  available.
- UIX settings, FTP status, NTP, Discord Presence, skins, cache management, and
  translations are integrated into the dashboard UI.

## FTP server

FTP starts once the dashboard drives and network are ready. It defaults to
`xbox` / `xbox` on port 21 and can be configured or disabled in
`E:\TDATA\fffe0000\UIXLite\config.ini`.

- Exposes `C`, `D`, `E`, `F`, `G`, `X`, `Y`, and `Z`, plus supported
  `E2`/`F2`/`G2` second-HDD volumes. The optical drive appears only when media
  is present.
- Supports active and passive mode, upload/download, `REST` resume through
  offset `4,294,967,294`, directory creation/removal, deletion, and
  `RNFR`/`RNTO` rename.
- Reports actual FATX attributes and last-write dates. Unix-style listings use
  the time for recent entries and the year for entries older than six months.
- Uses tested 32 KiB data-socket buffering. Hardware testing reached about
  6.2 MiB/s; emulator networking performance varies.
- Writes normal uploads to a same-directory temporary file and replaces the
  destination only after successful completion, preventing a failed transfer
  from truncating an existing file.
- Reports insufficient disk space separately from data-connection failures.
- Restricts active-mode `PORT` targets to RFC 1918 private networks.

FTP is intentionally plain text and exposes writable partitions. Use it only
on a trusted private network.

## Disc-image support

- Launcher folders containing `default.iso` or `default.cci` can boot through
  the Cerbios virtual DVD drive.
- ISO and CCI metadata readers can retrieve a title ID and title name and can
  export the embedded `$$XTIMAGE` without mounting the image.
- CCI metadata access supports LZ4-compressed data.
- A stale Cerbios virtual disc is detached when the dashboard starts after a
  mounted title exits.
- Keep only the intended disc image in a title folder; ambiguous folders with
  both an ISO and CCI may cause an external attacher XBE to select the wrong
  image.

Cerbios build 8008 or newer is required for disc-image launching.

## Network services

### Discord Presence relay

`theConfig.SendDiscordRelayAsync(titleID)` sends presence updates without
blocking the XAP. UDP, TCP, and WebSocket delivery all run on a worker thread;
UDP additionally waits for its overlapped `WSASendTo` operation to complete
before reporting success. Relay state is available through
`GetDiscordRelayStatus()`.

The relay is configured under `[DiscordRelay]` in
`E:\TDATA\fffe0000\UIXLite\config.ini`.
Ports 1101, 1102, and 1103 select WebSocket, UDP, and TCP respectively.
`PresenceEnabled` is controlled by the dashboard's **Enable Discord Presence**
setting.

### Network time

`theConfig.SyncNetworkTimeAsync()` resolves either a hostname or IPv4 address,
queries an SNTP server on a worker thread, validates the reply, and updates the
system UTC clock. It defaults to `time.nist.gov` on standard NTP port 123 and
preserves the dashboard's timezone and daylight-saving settings. Status is
available through `GetNetworkTimeStatus()`.

Boolean configuration values accept `true`/`false`, `yes`/`no`, `on`/`off`,
and `1`/`0`.

## `theConfig` additions

The patch exposes XAP-callable functions for:

- applying the selected skin;
- launching and reading ISO/CCI disc images;
- retrieving XBE and disc-image title IDs and names;
- exporting XBE and disc-image title artwork;
- checking, copying, moving, deleting, and creating files and directories;
- creating `TitleName=<name>` metadata files;
- clearing the dashboard, X, Y, and Z caches;
- reading the console's IPv4 address;
- sending Discord relay updates and reading their status; and
- synchronizing network time and reading its status.

See [`Patches/README.md`](Patches/README.md) for exact function names,
signatures, return values, and diagnostics.

## XOnline Dashboard patch

The separate hash-locked XOnline patcher has selectable `colors`, `textures`,
`video720`, `redirect`, and `fontpath` features.

- Reads the same `[default] CurrentSkin` setting and skin directory as the 5960
  dashboard when XOnline Dashboard starts.
- Applies matching `Color`, `ColorA`, and `ColorB` material values.
- Replaces matching textures normally loaded from `C:\xodash\Media\Xbx`;
  missing
  overrides retain the retail texture.
- Enables matching 720p device and UI dimensions.
- Corrects the retail Xbox font path to the partition root.
- Returns to `C:\xb0xdash.xbe` when present and otherwise preserves the normal
  `xboxdash.xbe` return path and launch arguments.
- Does not redirect `.xbg` models and does not live-switch skins while inside
  XOnline Dashboard.

The command-line patcher and `xonlinedash-uixlite.vxp` both validate the
supported retail XBE before making changes.

## Stability and compatibility fixes

- Corrected XIP signature-hook control flow that could corrupt execution and
  cause non-English dashboards or modified XIPs to boot-loop.
- Confirmed English and Japanese startup on both xemu and real hardware after
  the fix; the translation set also includes German, French, Spanish, Italian,
  Korean, Traditional Chinese, and Portuguese.
- Moved FTP startup until after dashboard drive mapping and configuration paths
  are available.
- Fixed FTP enable/disable parsing, upload failure handling, timestamps,
  attributes, large resume offsets, and active/passive data-socket behavior.
- Fixed UDP relay readiness and completion handling and moved asynchronous
  relay and NTP work off the XAP caller thread.
- Added cache clearing for X, Y, Z, and `E:\CACHE`; the dashboard's locked
  `LocalCache00.bin` is intentionally retained and does not count as failure.
- Expanded live texture and mesh tracking and restored retail archive paths
  when skin overrides are removed.
- Improved Info Panel show/hide ownership across the stock menu, REZN8 menu,
  levels, and screensaver.

## Installation

### Pre-patched release

Copy the supplied `xboxdash.xbe` and `xboxdashdata.185ead00\` files over an
existing complete retail 5960 installation on `C:`. Do not delete untouched
retail dashboard files, fonts, or `xodash` content.

### Build from source

Use Python 3 with `Patches/patch.py`, or open
`Patches/bin/xboxdash-5960-uixlite.vxp` in the latest VisualXIP. The separate
XOnline package is `Patches/xonline/bin/xonlinedash-uixlite.vxp`.

Both patchers validate the expected retail input and write a new output file.
See [`Patches/README.md`](Patches/README.md) and
[`Patches/xonline/README.md`](Patches/xonline/README.md) for complete build and
patching instructions.

## Current status and known limitations

- This remains a testing release. The dashboard has booted in English and
  Japanese on xemu and real hardware, and core skin, FTP, disc-image, relay,
  NTP, 720p, and Insignia paths have received targeted testing rather than a
  long-duration full-feature soak.
- XAP `onLoad()` callbacks on an `Inline` contained by a `Layer` can leave the
  retail VM's temporary runner pointer stale. UIX Lite avoids this fragile path
  by polling `children.length()` from a persistent behavior where needed.
- A boot worker initializes the extended F/G partition subsystem without
  blocking dashboard rendering or input. Native completion is exposed to the
  XAP so completed warm-up suppresses the indexing message without discarding
  queued scans, launches, directory loads, cache refreshes, or UDATA work. If
  an extended partition is used early, the existing indexing message covers
  the remaining delay. The tested initialization took about 49 seconds;
  filesystem operations issued before it finishes may still wait behind the
  storage stack's shared initialization lock.
- Skin material names are shared. Changing a material's RGBA values affects
  every object that uses that material, including intentionally translucent
  materials.
- XOnline skins are loaded only when entering XOnline Dashboard; they are not
  switched live within it.
- xemu networking can be substantially slower or less consistent than real
  hardware, especially with bridged or passive transfers.

## Requirements

- A modchip or softmod that does not rely on the Microsoft dashboard files to
  boot.
- A complete retail 5960 Microsoft dashboard installation.
- Cerbios build 8008 or newer for disc-image launching.
- Python 3 or the latest VisualXIP when applying the patches yourself.
- LLVM only when rebuilding the C payloads from source.

## License

The patch source under [`Patches/`](Patches) and the repository scripts are
licensed under the GNU General Public License v3.0 or later. See
[`LICENSE`](LICENSE).

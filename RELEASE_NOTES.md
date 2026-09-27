# UIX Lite v0.6 (testing)

**Heads up:** this is a testing build. Back up your current dashboard before
installing, and expect rough edges. It runs on a modded Xbox with a working
5960 MS dashboard already in `C:\`.

The headline: the patches are now built **from source**. Reverse engineering
the 5960 dashboard from scratch (the work that became Theseus / UIX Desktop)
means we finally understand how the dashboard actually works, so UIX Lite's
changes live in [`Patches/`](Patches) as readable C and a Python patcher
instead of opaque byte diffs. That also let us add things the old patches
never could, the big one being an FTP server that runs inside the dashboard
itself.

## What's new

- **In-dashboard FTP server.** Push and pull files over the network straight
  to the running dashboard, no homebrew app in the middle. It comes up on its
  own once the network is ready and coexists with Insignia sign-in.
  - Log in `xbox` / `xbox` (configurable).
  - Drives appear as the file browser's letters (`C`, `E`, `F`, `G`, `D` for
    a disc, `E2`/`F2`/`G2` for a second HDD), not the raw system letters.
  - Upload, download, resume, directory listing, make/remove/delete.
  - Configure it in `UIX Configs\config.ini`:
    ```
    [FTP]
    Enabled=1
    User=xbox
    Pass=xbox
    Port=21
    ```
- **`theConfig.GetIPAddress()`** for skins, so a scene can show the console's
  LAN IP the same way it shows the dashboard version. Handy for knowing where
  to point your FTP client.
- **Patches rebuilt from source.** `Patches/patch.py` applies everything to
  your own retail 5960 `xboxdash.xbe`; no XBE is distributed in the repo. Pick
  features with `--only` / `--skip`.

## The full patch set

| Feature | What it does |
|---|---|
| `drives` | Maps the extra HDD partitions to drive letters |
| `video720` | 1280x720 progressive on an HDTV pack when 720p + widescreen are on |
| `xipsig` | Loads modified XIPs (skips the signature/digest checks) |
| `dvdregion` | Plays DVDs from any region |
| `skins` | Theseus-style skins, switchable live from Settings > Skins |
| `discimages` | Boots `default.iso` / `default.cci` title folders on Cerbios |
| `ftp` | The FTP server above |

## Installing

Copy `xboxdash.xbe`, `xboxdashdata.185ead00\` and `UIX Configs\` to the root
of your `C:` partition, over your existing 5960 install. Leave your fonts,
`Xbox.xtf` / `XBox Book.xtf` and `xodash` alone. See `INSTALL.txt` in the
build for the details.

Building it yourself from source: see [`Patches/README.md`](Patches/README.md).

## Known issues / please test

- The from-source build as a whole hasn't had a full hardware soak yet;
  individual features (FTP upload/download, disc-image booting, skins, 720p,
  Insignia sign-in) have been tested piecemeal. Report anything that misbehaves.
- FTP resume past 2 GB isn't handled yet (fresh transfers of any size are fine).
- The skin subsystem still has mesh (`.xm`) overrides and the Setup-driven
  glow colors (Key / EggGlow) to finish.

## Requirements

- A modchip or softmod that does **not** boot from the dashboard files.
- A working 5960 MS dashboard in `C:\`.
- Cerbios (build 8008+) for disc-image launching.

## License

The patch source in [`Patches/`](Patches) and the scripts in this repo are
GPL-3.0-or-later. See [LICENSE](LICENSE).

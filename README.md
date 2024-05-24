# UIX-Ultra-Lite
Modern/Re-Implemented Patches and Scripts for the Xbox Dashboard #5960
* Completely based on old school patches and hex edits, not a source modification. So you don’t need to feel "dirty" about using it.

## What is UIX Ultra Lite
As stated in the title above UIX Ultra Lite is a collection of patches and XIP edits that adds some of the features of UIX to the stock 5960 MS Dash. 
Some of these features include; 
* A customizable Launcher to launch titles from your Xbox hard disk.
* The ability to customize the Main Menu items.
* An option to select the tHc Main Orb style.
* Define up to four QuickLaunch titles.
* Modify the Xbox Live item to say Insignia.
* All configuration changes can be made in-dashboard via Settings>Configuration.

## Installation
* Since UIX Ultra Lite is merely patches for the MS Dash, you must first have a complete working copy of the 5960 MS Dash installed in the root of your C partition before attempting either of the two methods below.

## How-To (Pre-Patched)

* DO NOT USE ON AN UNMODIFIED XBOX. It won’t work, and you'll brick your Xbox. Make sure you have a modchip or softmod installed that doesn’t rely on the dashboard files to boot.
* DO NOT DELETE YOUR ORIGINAL MS DASHBOARD FILES. SIMPLY REPLACE THE EXISTING FILES WITH THE MODIFIED FILES ONLY. KEEP ANY UNMODIFIED FILES.
* Download the latest xboxdashdata.185ead00-vXX-XX-XX.zip from the assets in the pre-patched release.
* Extract and Copy the pre-patched 5960 Dashboard files via ftp to the root of the C partition on your Xbox.

## How-To (Manual)

* DO NOT USE ON AN UNMODIFIED XBOX. It won’t work, and you'll brick your Xbox. Make sure you have a modchip or softmod installed that doesnt rely on the dashboard files to boot.
* DO NOT DELETE YOUR ORIGINAL MS DASHBOARD FILES. SIMPLY REPLACE THE EXISTING FILES WITH THE MODIFIED FILES ONLY. KEEP ANY UNMODIFED FILES.
* Download Visual XIP from the Tools page.
* Copy the unmodified 5960 Dashboard files to a directory on your computer.
* Use Visual XIP or the Binary Patcher to patch the xboxdash.xbe to allow modified xips and to add F & G partition support.
* Use Visual XIP or WinXip to modify the xips in your xboxdashdata.185ead00 with the updated source files from the github repository.
* Copy your now "modified" 5960 Dashboard files via ftp, including config.xbx, to the root of the C partition on your Xbox.

## Configuration and Use

* Using the settings in the Example config.xbx below will cause your xbox to appear as stock on boot until you go to Settings and find the Launcher and Configuration menu items. The Launcher can also be accessed by pressing YX.
* You can customize the Main Menu, Launcher Menu, and other settings in-dash by navigating to Settings>Configuration.
* When configuring the Main Menu, advanced users can enable Advanced Mode. When this is enabled, selecting a setting will bring up the xbox onscreen keyboard for custom input instead of using the built-in toggles.
* When configuring the Launcher, you may define both the “Title” for a content type and the relative “Path(s)” to the content for up to eight launcher menu items. The Launcher will scan the relative paths provided on each partition and compile a list of the subdirectories found. The name of these subdirectories should correspond to the name of the title and contain the title's default.xbe. Use a semicolon as a separator when defining multiple relative paths to like content.
* You can fast scroll most menus by pressing Left/Right on the directional pad to PageUp/PageDown.
* To re-launch the last title launched, simply press the X button while in the Launcher menu.
* You can assign a QuickLaunch path to each of the A, B, X, and Y buttons. To active the QuickLaunch feature, while in the Main Menu hold both triggers and press the assigned button.

 NOTE: The xbox onscreen keyboard does not allow you to enter an empty text string. Therefore, to delete an entry just enter an single "x" and select Done.
 NOTE: The xbox onscreen keyboard is limited to 31 charaters. So, if you need more characters, just enter as much as you can and select Done. Then edit it again and some of the text will be moved into the title bar and you can add more text.
 

## Example config.xbx
```
[default]
MainOrbStyle=Stock
ShowInsignia=false
ConfigPanelIcon=Globe
LauncherOrbIcon=Xbox
UsetHcScreenSaver=false

[MainMenu]
MainMenuItems=4
Button1Text=MEMORY
Button1Action=GoToMemory()
Button2Text=MUSIC
Button2Action=GoToMusic()
Button3Text=XONLINE
Button3Action=GoToXOnline()
Button4Text=LAUNCHER
Button4Action=GoToLauncher()
ButtonYXAction=GoToLauncher()
AdvancedMode=false

[LauncherMenu]
Title0=Applications
Path0=Applications;Apps
Title1=Dashboards
Path1=Dashboards
Title2=Games
Path2=Games
Title3=Emulators
Path3=Emulators;Emus
Title4=Homebrew
Path4=Homebrew
Title5=
Path5=
Title6=
Path6=
Title7=
Path7=

[ShowInSettings]
Memory=false
Music=false
XOnline=false
Launcher=true

[QuickLaunch]
QuickLaunchA=E:\Dashboards\UnleashX\default.xbe
QuickLaunchB=
QuickLaunchX=
QuickLaunchY=

```

## Special Thanks
* The original TeamUIX and members of xboxdash[.]net
* The Xbox-Scene Community
* Insignia Live; Without you guys making an effort to revive Xbox Live, we wouldn't have cared if this dash was compatible.

## Family Tree

## tHc/tHc Lite
* Non-Source Based 4920 Modification.
* Additional, "unavailable-in-source" (F and G Support, for example. The assembly patches were never public, installer only.) binary patches.
* XIP and XAP Modifications.
* Live Support Stripped from XIPS. 

## UIX Ultra Lite
* Non XDK Source Based 5960 Modification
* Re-Implementation and open sourcing of patches from 4920.
* Re-Implementation of XIP Modifications, adapting to changes between 4920 and 5960.
* Binary Patcher is written in Go, and is cross platform. It's a simple command line tool, that will patch the files for you using the bsdiff format. :D


# Why?

In the early 2000's we didn't have access to source code, thus projects like UIX and UIX Lite didn't exist. What we did have was a sub-scene, within the Xbox scene that focused on modifications to the retail dashboards files. The work of these people, was the foundation for what UIX eventually became, as well as the original TeamUIX. So, we're going into the archives and taking a look at patches and mods from our friends Gcue, fuckdb, Vulgasprofanum, the original tHc, and xboxdash[.]net community and implementing them in a modern, live enabled, dashboard.

The end goal, will be to provide a legal, open source, distributable patch set and tutorials for ease of use.

A modified console will be required to run this, but we will be utilizing 20 years of modification techniques to make this as easy as possible for the end user.

# To-Do

* Launch Games From the F and G Partitions. UPDATE: Added functionality by currently un-credited genius.
* Figure out what else we can add in the binaries whitespace without breaking it.

# Non-Features
One of the caveats of turning back the clock here, is we won't be touching any source code. It's leaked, old, outdated and will not allow us to have a proper setup for systems intending to connect to network services.

* No FTP Server.
* No Web Server.
* No Game Icons In HDD Loader (Maybe..)

# POC

We successfully got the HDD Loader from tHc Lite (Which is why the POC Video here is called tHc Ultra-Lite) to launch XBE's from within 5960.

[![tHc Ultra Lite POC](http://img.youtube.com/vi/IlFVf--V0Ac/0.jpg)](https://www.youtube.com/watch?v=IlFVf--V0Ac)

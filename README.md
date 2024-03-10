# UIX-Ultra-Lite
Modern/Re-Implemented Patches and Scripts for the Xbox Dashboard #5960

## How-To

* DO NOT USE ON AN UNMODIFIED XBOX. It wont work, and you'll brick your Xbox. Make sure you have a modchip or softmod installed that doesnt rely on the dashboard files to boot.
* Download Visual XIP from the Tools page.
* Copy the unmodified 5960 Dashboard files to a directory on your computer.
* Use Visual XIP or the Binary Patcher to patch the xboxdash.xbe to allow modified xips
* Use Visual XIP or WinXip to modify the xips in xboxdashdata.185ead00 with the updated xap source files or download the premodified xips from the releases page.
* Copy the "modified" 5960 Dashboard files via ftp, including config.xbx, to the root of your C drive on your Xbox.

## Modifications

* Completely based on old school patches and hex edits, not a source modification. So you dont need to feel "dirty" about using it.
* Removes XIP signature checks.
* Modifies the Orb to the tHc Orb. (Set UseThcOrb to false in the config.xbx for a more stock look.)
* Modifies Xbox Live tab to say Insignia when the ShowInsignia option is set to true.

## Example config.xbx
```
[default]
MainOrbStyle=Stock
ShowInsignia=false
ConfigPanelIcon=Globe
LauncherOrbIcon=Xbox

[MainMenu]
MainMenuItems=4
Button1Text=MEMORY
Button1Action=GoToMemory()
Button2Text=MUSIC
Button2Action=GoToMusic()
Button3Text=XONLINE
Button3Action=GoToXOnline()
ButtonYXAction=GoToLauncher()

[LauncherMenu]
Title0=Applications
Path0=Apps
Title1=Dashboards
Path1=Dashboards
Title2=Games
Path2=Games
Title3=Emulators
Path3=Emus
Title4=
Path4=

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

## Usage
* Currently does not see F or G partitions, so you will need to add [shortcuts](https://github.com/BigJx/UIX-Ultra-Lite/blob/main/Shortcuts/README.md) to the files on those partitions to E:\Shortcuts\\[SectionName]\\[TitleName].(i.e. E:\Shortcuts\Games\Fable)


## Family Tree

## tHc/tHc Lite
* Non-Source Based 4920 Modification.
* Additional, "unavailable-in-source" (F and G Support, for example. The assembly patches were never public, installer only.) binary patches.
* XIP and XAP Modifications.
* Live Support Stripped from XIPS. 

## UIX Ultra Lite
* Non-Source Based 5960 Modification
* Re-Implementation and open sourcing of Binary Patches from 4920.
* Re-Implementation of XIP Modifications, adapting to changes between 4920 and 5960.
* Binary Patcher is written in Go, and is cross platform. It's a simple command line tool, that will patch the files for you using the bsdiff format. :D


# Why?

In the early 2000's we didn't have access to source code, thus projects like UIX and UIX Lite didn't exist. What we did have was a sub-scene, within the Xbox scene that focused on modifications to the retail dashboards files. The work of these people, was the foundation for what UIX eventually became, as well as the original TeamUIX. So, we're going into the archives and taking a look at patches and mods from our friends Gcue, fuckdb, Vulgasprofanum, the original tHc, and xboxdash[.]net community and implementing them in a modern, live enabled, dashboard.

The end goal, will be to provide a legal, open source, distributable patch set and tutorials for ease of use.

A modified console will be required to run this, but we will be utilizing 20 years of modification techniques to make this as easy as possible for the end user.

# To-Do

* Launch Games From the F and G Partitions.

# Non-Features
One of the caveats of turning back the clock here, is we won't be touching any source code. It's leaked, old, outdated and will not allow us to have a proper setup for systems intending to connect to network services.

* No FTP Server.
* No Web Server.
* No Game Icons In HDD Loader (Maybe..)

# POC

We successfully got the HDD Loader from tHc Lite (Which is why the POC Video here is called tHc Ultra-Lite) to launch XBE's from within 5960.

[![tHc Ultra Lite POC](http://img.youtube.com/vi/IlFVf--V0Ac/0.jpg)](https://www.youtube.com/watch?v=IlFVf--V0Ac)

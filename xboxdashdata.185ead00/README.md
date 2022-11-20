# What are these files?
The xbox dashboard uses a custom flavor of VRML, which is the Virtual Reality Markdown Language to create its menus. As such we are able to make modifications to the menus and utilize built in functions.

* The folders in this Git represent the files you will be modifying. 

# Pre-Requisites

* A modded xbox, or a mod that doesn't stop you from using the xbox dashboard. Ideally you will be using a method with a shadowc, or a hardmod. We are NOT responsible if you screw anything up.
* Xbox Dashboard 5960 that utilizes the folder xboxdashdata.185ead00 
* WinXIP v0.88 you can find this with a quick google search.
* [TeamUIX's XBE Shortcut Maker](https://consolemods.org/wiki/Xbox:XBE_Shortcut_Maker)
* Your favorite Hex Editor; I perfer HxD
* The files in this GIT.
* Fifteen minutes of your time.

# How-To
## Patch the XBE

Using your favorite Hex editor, do the following:
Search for:
```
0F 84 94 00 00 00 8B 43 14 3B 82 80 00 00 00 0F 83 85 00 00 00 8D 34 80 8D BC B2 84 00 00 00 B9 05 00 00 00 8D 74 24 10 33 D2 F3 A7 75 6C 8B
```
Replace with:
```
0F 84 94 00 00 00 8B 43 14 3B 82 80 00 00 00 90 90 90 90 90 90 8D 34 80 8D BC B2 84 00 00 00 B9 05 00 00 00 8D 74 24 10 33 D2 F3 A7 90 90 8B
```
## Patch the XIP Files
### default.xip
* Open default.xip in WinXIP
* Insert harddrive.xap (Edit > Insert)
* Insert default.xap (Edit > Insert) Replacing the existing file.
* Insert settings3.xap (Edit > Insert) Replacing the existing file.

### Modify additional XAP Files in default.XIP.
We will be modifying the following files within default.xip. Right click > Edit each file individually, use notepad to edit them. Save the file, and continue onto the next one.
* AccountSelection.xap
* LiveToday.xap
* dvd.xap
* memory3.xap
* music2.xap
* These files require unloadable false to be added to their "archive string". The easiest way to get this done is to open each file listed above, and search for .xip, modifying the code like in the example below. Once your changes are made, save the xap file. WinXIP has created a temporary working folder, where these get saved.
* When all of these are done, save the XIP by clicking the save icon in the top left of WinXIP. This will re-pack all of the files you just modified.
```
    archive "AccountSelection.xip"

    children
    [
```
Replace With:
```
    archive "AccountSelection.xip"
    unloadable false

    children
    [
```
* Transfer default.xip to C:\xboxdashdata.185ead00\ on your xbox harddrive.


### mainmenu5.xip
* Open mainmenu5.xip in WinXIP
* Insert default.xap (Edit > Insert) replacing the original file.
* Insert OrbCell-FACES.xm (Edit > Insert)
* Insert Soundtrack_Backing.xbx (Edit > Insert)
* Insert orbcellwall.xbx (Edit > Insert)
* Save the file, and transfer it to C:\xboxdashdata.185ead00\ on your xbox harddrive.

### music_playedit2.xip
* Open music_playedit2.xip in WinXIP
* Insert default2.xap (Edit > Insert)
* Insert MOrbCell-FACES.xm (Edit > Insert)
* Insert Morbcellwall (Edit > Insert)
* Save the file, and transfer it to C:\xboxdashdata.185ead00\ on your xbox harddrive.

## config.xbx
Since we're using harddrive.xap, we also need to borrow tHc/UIX Lite's config file structure.
* Upload config.xbx to the root of C:\.

## Shortcuts
By default the dashboard can see data on the E partition, since we do not currently have a patch for F and G support you can utilize the XBE Shortcut Maker and install shortcuts to using the paths you set in the config above. It's not recommended, to use the same titleID of the game you're launching. So change the name, but keep XBMC's default ID when creating the shortcut. This mod sees titles based on folder name, not the name of the XBE anyway. Upload the newly created shortcut using the example structure below.

```
E:\GamesPathHere\GameNameHere\default.xbe
E:\AppsPathHere\AppNameHere\default.xbe
E:\DashboardsPathHere\DashNameHere\default.xbe
E:\EmulatorsPathHere\EmulatorNameHere\default.xbe
```
and they will appear in the menu.

To launch the menu, we hijacked NoisyCam, which would make the dashboard float. Activate the HDD Menu by pressing L and R triggers, and while holding press Y and X. NoisyCam can still be enabled from the Xbox Live dashboard. :)

Congrats! You should have a working, stock dashboard, with a harddrive menu.

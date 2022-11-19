# What are these files?
The xbox dashboard uses a custom flavor of VRML, which is the Virtual Reality Markdown Language to create its menus. As such we are able to make modifications to the menus and utilize built in functions.

* The folders in this Git represent the files you will be modifying. 

# Pre-Requisites

* A modded xbox, that doesn't require an alternative dashboard. Ideally you will be using a method with a shadowc, or a hardmod. We are NOT responsible if you screw anything up.
* Xbox Dashboard 5960 that utilizes the folder xboxdashdata.185ead00 
* WinXIP v0.88 you can find this with a quick google search.
* [TeamUIX's XBE Shortcut Maker](https://consolemods.org/wiki/Xbox:XBE_Shortcut_Maker)
* Your favorite Hex Editor. 
* The files in this GIT.

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

* Right-Click Edit AccountSelection.xap
Find:
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
* Right-Click Edit LiveToday.xap
Find:
```
    archive "PasscodeVerify.xip"

    children
    [
```
Replace With:
```
    archive "PasscodeVerify.xip"
    unloadable false

    children
    [
```
* Right-Click Edit dvd.xap
Find:
```
    archive "dvd.xip"
    fade false

    children
    [
```
Replace With:
```
    archive "dvd.xip"
    unloadable false
    fade false

    children
    [
```
* Right-Click Edit memory3.xap
Find:
```
    archive "Memory_Files2.xip"

    children
    [
```
Replace With:
```
    archive "Memory_Files2.xip"
    unloadable false

    children
    [
```
* Right-Click Edit music2.xap
You're going to add:
```
    unloadable false
```
Under each "archive" entry below.
Find:
```
    archive "Music_Copy3.xip"

    children
    [
```
Find:
```
    archive "Music_PlayEdit2.xip"

    children
    [
```
* Right-Click Edit settings3.xap:
You're going to add:
```
    unloadable false
```
Under each "archive" entry below; See Settings_Clock.xip example below for the format.
Find:
```
    archive "Settings_Clock.xip"

    children
    [
```
Replace With:
```
    archive "Settings_Clock.xip"
    unloadable false

    children
    [
```
Find:
```
    archive "Settings_Timezone.xip"

    children
    [
```
Find:
```
    archive "Settings_List.xip"

    children
    [
```
Find:
```
    archive "Settings_Panel.xip"

    children
    [
```
Find:
```
    archive "Settings_Language.xip"

    children
    [
```
Find:
```
    archive "Settings_Video.xip"

    children
    [
```
Find:
```
    archive "Settings3.xip"

    children
    [
```

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

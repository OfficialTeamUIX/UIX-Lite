# What are these files?
The xbox dashboard uses a custom flavor of VRML, which is the Virtual Reality Markdown Language to create its menus. As such we are able to make modifications to the menus and utilize built in functions.

# Pre-Requisites

* A modded xbox, that doesn't require an alternative dashboard. Ideally you will be using a method with a shadowc, or a hardmod. We are NOT responsible if you screw anything up.
* Xbox Dashboard 5960 (xboxdashdata.185ead00) 
* WinXIP v0.88
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
* Right-Click Edit default.xap
Find this:
```
DEF theTranslator Translator

////////////////////////////////////////////////////////////////////////////

DEF theMusicInline Inline
```
Replace with:
```
///////////////////////////////////////////////////////////////////
DEF theMusicPlayWithSubsInline Inline
{
    visible false
    url "harddrive.xap"

    function onLoad()
    {
        theMusicPlayWithSubsInline.children[0].theMusicPlayWithSubs.GoTo();
    }
}

function GoToMusicPlayWithSubs()
{
    if (theMusicPlayWithSubsInline.visible)
        theMusicPlayWithSubsInline.children[0].theMusicPlayWithSubs.GoTo();
    else
        theMusicPlayWithSubsInline.visible = true;
}
////////////////////////////////////////////////////////////////////
```
Find this:
```
        function OnKeyVerified()
        {
            theConfig.ToggleNoisyCamera();
        } 
```
Replace with:
```
        function OnKeyVerified()
        {
            //theConfig.ToggleNoisyCamera();
	    theGamesMenuIn.Play();
	    GoToMusicPlayWithSubs();
        }  
```
Find this:
```
function initialize()
{
    bBackToDVDPlayer = false;
```
Replace with:
```
// All new var's below here...

var LeftTrigger;
var RightTrigger;
var RightThumb;
var LeftThumb;

var CurrentViewpoint;
var CurrentAltViewpoint;

var theInLine;
var currentControlType;
var previousControlType;
var TotalSections;
var SectionName;
var SectionPath;
var SubMenuItem1;
var SubMenuItem2;
var SubMenuItem3;
var SubMenuItem4;

var bMusicPlayerReady;
var currentMusicPlayerMode;

var quicklaunch;
var gamelaunch;
var launchPath;
var launchXbe;

var CurrentGameBoardAltViewpoint;

function initialize()
{

    quicklaunch = false;
    gamelaunch = false;



    RightThumb = false;
    LeftThumb = false;
    LeftTrigger = false;
    RightTrigger = false;
    bBackToDVDPlayer = false;
```
Find this:
```
function UnblockMemoryUnitEnumeration()
{
    theMemoryMonitor.enumerationOn = false;
}
```
APPEND, after the last bracket.
```
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////// Added Functions for HDD Menu
////////////////////////////////////////////////////////////////////////////////////////////////////////////////



function ReturnInteger(x) // data read from xbx is read as a string - this will convert that string value to an integer
{
    var a = x;
    var b = Math.abs(a);
    if(a.charCodeAt(0) == 45)
    {
        var c = b * 2;
        b = b - c;
    }
    return b;
}

function FillMenuList(x) // search c,e,f,g for a relative path's contents to use to fill a menu
{
    var FolderName = x;
    var c = new Folder; //def a new folder
    c.path = "Y:\\" + FolderName; //def it's path
    var cSize = c.subFolders.length(); //find the number of subfolders in it
    var cTitles = new Array(cSize); //setup a loop to put it's contents into an array
        if(cSize > 0)
        {
         for(var cLoop = 0; cLoop < cSize; cLoop = cLoop + 1)
         {
             cTitles[cLoop] = c.subFolders[cLoop].name;
         }
        }
    var e = new Folder; //repeat above for all drives
    e.path = "C:\\" + FolderName;
    var eSize = e.subFolders.length();
    var eTitles = new Array(eSize);
        if(eSize > 0)
        {
         for(var eLoop = 0; eLoop < eSize; eLoop = eLoop + 1)
         {
             eTitles[eLoop] = e.subFolders[eLoop].name;
         }
        }

    var mA = eTitles.concat(fTitles);//keep merging the arrays till you have one big one
    var oA = mA.concat(cTitles);
    var nA = oA.concat(gTitles);
    nA = nA.sort();//sort the one big array into alphabetical order
    return nA;//return the array
}

DEF theLaunchGameLevel Level
{
    function OnActivate()
    {
        theScreenSaver.enabled = true;
    }
    function OnArrival()
    {
        if(quicklaunch)
        {
            launch(launchXbe, launchPath);
        }
        else if(gamelaunch)
        {
            launch(launchXbe, launchPath);
        }
    }
}
function LaunchItem(x,y)
{
    var RelativePath = x;
    var FolderName = y;
       var checkc = theConfig.NtFileExists( "\\Device\\Harddisk0\\Partition2\\" + RelativePath + "\\" + FolderName + "\\default.xbe" );
       var checke = theConfig.NtFileExists( "\\Device\\Harddisk0\\Partition1\\" + RelativePath + "\\" + FolderName + "\\default.xbe" );

       
       if(checkc == true)
       {
        launchPath = "\\Device\\Harddisk0\\Partition2\\" + RelativePath + "\\" + FolderName;
        launchXbe = "default.xbe";
        theLaunchGameLevel.GoTo();
       }
       else if(checke == true)
       {
        launchPath = "\\Device\\Harddisk0\\Partition1\\" + RelativePath + "\\" + FolderName;
        launchXbe = "default.xbe";
        theLaunchGameLevel.GoTo();
       }


}






////////////////////////////////////////////////////////////////////////////


function EnableCurrentAlternateViewpoint()
{
    CurrentAltViewpoint.isBound = true;
}

function DisableCurrentAlternateViewpoint()
{
    CurrentViewpoint.isBound = true;
}

DEF theMainMenuAlternateViewpoint Viewpoint
{
        fieldOfView 1.755000
        orientation -0.177400 -1.983500 -0.036250 -0.045440
        position -15.180000 -112.299999 174.300003
        jump false
}





function GetSubmenuText()
{
    var info = new Settings;
    info.file = "Y:\\config.xbx";
    SubMenuItem1 = info.GetValue("Memory Text");
    SubMenuItem2 = info.GetValue("Music Text");
    SubMenuItem3 = info.GetValue("HardDrive Text");
    SubMenuItem4 = info.GetValue("Settings Text");
}

function GetConfigInfo()
{
    var info = new Settings;
    info.file = "Y:\\config.xbx";
    TotalSections = info.GetValue("Total Sections");
    return TotalSections;
}

function GetSectionTitles(x)
{
    var a = x;
    var info = new Settings;
    info.file = "Y:\\config.xbx";
    info.section = "section" + a;
    SectionName = info.GetValue("Title");
    return SectionName;
}

function GetSectionPaths(x)
{
    var a = x;
    var info = new Settings;
    info.file = "Y:\\config.xbx";
    info.section = "section" + a;
    SectionPath = info.GetValue("Path");
    return SectionPath;
}
```
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
Hopefully your were saving all of these files which WinXIP has now nicely stored in a temporary directory, and lit up the save icon in the toolbar. Save the file, and transfer it to C:\xboxdashdata.185ead00\ on your xbox harddrive.

### music_playedit2.xip
* Open music_playedit2.xip in WinXIP
* Insert default2.xap (Edit > Insert)
* Insert MOrbCell-FACES.xm (Edit > Insert)
* Insert Morbcellwall (Edit > Insert)
* Save the file, and transfer it to C:\xboxdashdata.185ead00\ on your xbox harddrive.

## config.xbx
Since we're using harddrive.xap, we also need to borrow tHc/UIX Lite's config file structure.
* Create a file called config.xbx and save it to the ROOT of C: on your xbox harddrive.
```
Total Sections=4

[section0]
Title=Applications
Path=Apps

[section1]
Title=Games
Path=Games

[section2]
Title=Dashboards
Path=Dashboards

[section3]
Title=Emulators
Path=Emus
```

## Shortcuts
By default the dashboard can see data on the E partition, since we do not currently have a patch for F and G support you can utilize the XBE Shortcut Maker and install shortcuts to using the paths you set in the config above. You can search a list of titleID's [here](https://github.com/jeltaqq/Xbox-Original-GameList/blob/master/Xbox%20Original%20GameList.tsv) so your save menu doesnt get cluttered with bogus save files.
```
E:\GamesPathHere\GameNameHere\default.xbe
E:\AppsPathHere\AppNameHere\default.xbe
E:\DashboardsPathHere\DashNameHere\default.xbe
E:\EmulatorsPathHere\EmulatorNameHere\default.xbe
```
and they will appear in the menu.

To launch the menu, we hijacked NoisyCam, which would make the dashboard float. Activate the HDD Menu by pressing L and R triggers, and while holding press Y and X. NoisyCam can still be enabled from the Xbox Live dashboard. :)

Congrats! You should have a working, stock dashboard, with a harddrive menu.
